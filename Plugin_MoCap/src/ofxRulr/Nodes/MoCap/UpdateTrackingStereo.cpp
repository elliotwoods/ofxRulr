#include "pch_Plugin_MoCap.h"
#include "UpdateTrackingStereo.h"

#include "ofxRulr/Nodes/GraphicsManager.h"

#include "MatchMarkers.h"

namespace ofxRulr {
	namespace Nodes {
		namespace MoCap {
			//----------
			UpdateTrackingStereo::UpdateTrackingStereo() {
				RULR_NODE_INIT_LISTENER;
				this->setIcon(Nodes::GraphicsManager::X().getIcon("MoCap::icon"));
			}

			//----------
			std::string UpdateTrackingStereo::getTypeName() const {
				return "MoCap::UpdateTrackingStereo";
			}

			//----------
			void UpdateTrackingStereo::init() {
				RULR_NODE_INSPECTOR_LISTENER;
				RULR_NODE_UPDATE_LISTENER;

				this->addInput<Body>();
				this->addInput<Procedure::Calibrate::StereoCalibrate>();

				this->threadPool = make_unique<Utils::ThreadPool>(2, 10);
				this->stereoSolvePnP = make_unique<StereoSolvePnP>();

				{
					auto input = this->addInput<MatchMarkers>("MatchMarkers A");
					input->onNewConnection += [this](shared_ptr<MatchMarkers> inputNode) {
						inputNode->onNewFrame.addListener([this](shared_ptr<MatchMarkersFrame> incomingFrame) {
							if (!this->threadPool->performAsync([this, incomingFrame]() {
								this->processFrame(incomingFrame, 0);
								this->processedFramesSinceLastAppFrame++;
							})) {
								this->droppedFramesSinceLastAppFrame++;
							}
						}, this);
					};
					input->onDeleteConnection += [this](shared_ptr<MatchMarkers> inputNode) {
						if (inputNode) {
							inputNode->onNewFrame.removeListeners(this);
						}
					};
				}

				{
					auto input = this->addInput<MatchMarkers>("MatchMarkers B");
					input->onNewConnection += [this](shared_ptr<MatchMarkers> inputNode) {
						inputNode->onNewFrame.addListener([this](shared_ptr<MatchMarkersFrame> incomingFrame) {
							if (!this->threadPool->performAsync([this, incomingFrame]() {
								this->processFrame(incomingFrame, 1);
								this->processedFramesSinceLastAppFrame++;
							})) {
								this->droppedFramesSinceLastAppFrame++;
							}
						}, this);
					};
					input->onDeleteConnection += [this](shared_ptr<MatchMarkers> inputNode) {
						if (inputNode) {
							inputNode->onNewFrame.removeListeners(this);
						}
					};
				}
				
			}

			//----------
			void UpdateTrackingStereo::update() {
				{
					auto lock = unique_lock<mutex>(this->bodyNodeMutex);
					this->bodyNode = this->getInput<Body>();
				}
				{
					auto lock = unique_lock<mutex>(this->stereoCalibrateNodeMutex);
					this->stereoCalibrateNode = this->getInput<Procedure::Calibrate::StereoCalibrate>();
				}

				while (this->computeTimeChannel.tryReceive(this->computeTime)) {}

				auto processedFramesPerSecond = (float)processedFramesSinceLastAppFrame.load() / ofGetLastFrameTime();
				this->processedFramesPerSecond = ofLerp(this->processedFramesPerSecond, processedFramesPerSecond, 0.1f);
				this->processedFramesSinceLastAppFrame.store(0);

				auto droppedFramesPerSecond = (float)droppedFramesSinceLastAppFrame.load() / ofGetLastFrameTime();
				this->droppedFramesPerSecond = ofLerp(this->droppedFramesPerSecond, droppedFramesPerSecond, 0.1f);
				this->droppedFramesSinceLastAppFrame.store(0);
			}

			//----------
			void UpdateTrackingStereo::populateInspector(ofxCvGui::InspectArguments & inspectArgs) {
				auto inspector = inspectArgs.inspector;
				inspector->addLiveValueHistory("Frames processed [Hz]", [this]() {
					return this->processedFramesPerSecond;
				});
				inspector->addLiveValueHistory("Dropped frames [Hz]", [this]() {
					return this->droppedFramesPerSecond;
				});
				inspector->addLiveValueHistory("Compute time [ms]", [this]() {
					return this->computeTime;
				});

			}

			//----------
			void UpdateTrackingStereo::processFrame(shared_ptr<MatchMarkersFrame> incomingFrame, bool cameraIndex) {
				vector<shared_ptr<MatchMarkersFrame>> markerTrackingResults;
				this->markerTrackingMutex.lock();
				{
					this->markerTracking[cameraIndex] = incomingFrame;
					markerTrackingResults = this->markerTracking;
				}
				this->markerTrackingMutex.unlock();

				if (cameraIndex != this->cameraWhichSendsSecond) {
					return;
				}

				//maybe we've got tracking!
				for (const auto & markerTrackingResult : markerTrackingResults) {
					if (!markerTrackingResult) {
						//not enough data to continue yet
						return;
					}
				}

				this->processCameraSet(markerTrackingResults, incomingFrame);

				//clean it out
				this->markerTrackingMutex.lock();
				{
					for (auto & markerTrackingResult : this->markerTracking) {
						markerTrackingResult.reset();
					}
				}
				this->markerTrackingMutex.unlock();
			}

			//----------
			void UpdateTrackingStereo::processCameraSet(vector<shared_ptr<MatchMarkersFrame>> markerTrackingResults, shared_ptr<MatchMarkersFrame> incomingFrame) {
				auto resultA = markerTrackingResults[0];
				auto resultB = markerTrackingResults[1];

				size_t countFinds = 0;
				for (const auto & markerTrackingResult : markerTrackingResults) {
					countFinds += markerTrackingResult->matchCount;
				}
				if (countFinds < 4) {
					return;
				}

				//get the body to update
				this->bodyNodeMutex.lock();
				auto bodyNode = this->bodyNode;
				this->bodyNodeMutex.unlock();
				if (!bodyNode) {
					return;
				}

				//get the stereo calibration node
				this->stereoCalibrateNodeMutex.lock();
				auto stereoCalibrateNode = this->stereoCalibrateNode;
				this->stereoCalibrateNodeMutex.unlock();
				if (!stereoCalibrateNode) {
					return;
				}

				//construct output
				auto outgoingFrame = make_shared<UpdateTrackingFrame>();
				outgoingFrame->incomingFrame = incomingFrame;
				bodyNode->getExtrinsics(outgoingFrame->modelViewRotationVector
					, outgoingFrame->modelViewTranslation);

				auto startTime = chrono::high_resolution_clock::now();
				bool success = stereoSolvePnP->solvePnPStereo(stereoCalibrateNode
					, resultA->matchedCentroids
					, resultB->matchedCentroids
					, resultA->matchedObjectSpacePoints
					, resultB->matchedObjectSpacePoints
					, outgoingFrame->modelViewRotationVector
					, outgoingFrame->modelViewTranslation
					, true);
				auto duration = chrono::high_resolution_clock::now() - startTime;
				auto micros = chrono::duration_cast<chrono::microseconds>(duration);
				this->computeTimeChannel.send((float)micros.count() / 1000.0f);

				if(success) {
					outgoingFrame->transform = bodyNode->updateTracking(outgoingFrame->modelViewRotationVector
						, outgoingFrame->modelViewTranslation
						, resultA->trackingWasLost && resultB->trackingWasLost);
					this->onNewFrame.notifyListeners(outgoingFrame);
				}
			}
		}
	}
}
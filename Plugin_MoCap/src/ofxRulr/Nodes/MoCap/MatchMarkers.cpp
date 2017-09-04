#include "pch_Plugin_MoCap.h"
#include "MatchMarkers.h"

#include "ofxRulr/Nodes/Item/Camera.h"

namespace ofxRulr {
	namespace Nodes {
		namespace MoCap {
#pragma mark Capture
			//----------
			MatchMarkers::Capture::Capture() {
				RULR_SERIALIZE_LISTENERS;
			}

			//----------
			string MatchMarkers::Capture::getDisplayString() const {
				stringstream ss;
				ss << "R : " << this->modelViewRotationVector << endl;
				ss << "T : " << this->modelViewTranslation << endl;
				ss << "error : " << this->reprojectionError;
				return ss.str();
			}

			//----------
			void MatchMarkers::Capture::serialize(Json::Value & json) {
				{
					auto & jsonModelViewRotationVector = json["modelViewRotationVector"];
					jsonModelViewRotationVector["x"] = this->modelViewRotationVector.x;
					jsonModelViewRotationVector["y"] = this->modelViewRotationVector.y;
					jsonModelViewRotationVector["z"] = this->modelViewRotationVector.z;
				}

				{
					auto & jsonModelViewTranslation = json["modelViewTranslation"];
					jsonModelViewTranslation["x"] = this->modelViewTranslation.x;
					jsonModelViewTranslation["y"] = this->modelViewTranslation.y;
					jsonModelViewTranslation["z"] = this->modelViewTranslation.z;
				}
			}

			//----------
			void MatchMarkers::Capture::deserialize(const Json::Value & json) {
				{
					const auto & jsonModelViewRotationVector = json["modelViewRotationVector"];
					this->modelViewRotationVector.x = jsonModelViewRotationVector["x"].asDouble();
					this->modelViewRotationVector.y = jsonModelViewRotationVector["y"].asDouble();
					this->modelViewRotationVector.z = jsonModelViewRotationVector["z"].asDouble();
				}

				{
					const auto & jsonModelViewTranslation = json["modelViewTranslation"];
					this->modelViewTranslation.x = jsonModelViewTranslation["x"].asDouble();
					this->modelViewTranslation.y = jsonModelViewTranslation["y"].asDouble();
					this->modelViewTranslation.z = jsonModelViewTranslation["z"].asDouble();
				}

				this->previewTransform = ofxCv::makeMatrix(cv::Mat(this->modelViewRotationVector)
					, cv::Mat(this->modelViewTranslation)).getInverse();
			}

#pragma mark MatchMarkers
			//----------
			MatchMarkers::MatchMarkers() {
				RULR_NODE_INIT_LISTENER;
			}

			//----------
			std::string MatchMarkers::getTypeName() const {
				return "MoCap::MatchMarkers";
			}

			//----------
			void MatchMarkers::init() {
				RULR_NODE_UPDATE_LISTENER;
				RULR_NODE_INSPECTOR_LISTENER;
				RULR_NODE_SERIALIZATION_LISTENERS;
				RULR_NODE_DRAW_WORLD_LISTENER;

				this->manageParameters(this->parameters);
				this->addInput<Body>();
				this->addInput<Item::Camera>();
			}

			//----------
			void MatchMarkers::update() {
				shared_ptr<Body::Description> bodyDescription;
				{
					auto bodyNode = this->getInput<Body>();
					if (bodyNode) {
						bodyDescription = bodyNode->getBodyDescription();
					}
				}

				shared_ptr<CameraDescription> cameraDescription;
				{
					auto cameraNode = this->getInput<Item::Camera>();
					if (cameraNode) {
						cameraDescription = make_shared<CameraDescription>();
						cameraDescription->cameraMatrix = cameraNode->getCameraMatrix();
						cameraDescription->distortionCoefficients = cameraNode->getDistortionCoefficients();
						cameraNode->getExtrinsics(cameraDescription->inverseRotationVector, cameraDescription->inverseTranslation, true);
						cameraDescription->viewProjectionMatrix = cameraNode->getViewInWorldSpace().getViewMatrix() * cameraNode->getViewInWorldSpace().getProjectionMatrix();
					}
				}

				{
					auto lock = unique_lock<mutex>(this->descriptionMutex);
					this->bodyDescription = bodyDescription;
					this->cameraDescription = cameraDescription;
				}

				{
					shared_ptr<Capture> newCapture;
					while (this->newCaptures.tryReceive(newCapture)) {
						this->captures.add(newCapture);
					}
				}

				{
					//tracking distance threshold should always be less than re-find threshold
					//because we use it as the 'sanity check' on the re-find (which is performed on lost tracking)
					if (this->parameters.trackingDistanceThreshold > this->parameters.refindTrackingThreshold) {
						this->parameters.refindTrackingThreshold = this->parameters.trackingDistanceThreshold;
					}
				}
			}

			//----------
			void MatchMarkers::drawWorld() {
				if (this->parameters.whenDraw.get() == WhenDrawWorld::Always
					|| this->parameters.whenDraw.get() == WhenDrawWorld::Selected && this->isBeingInspected()) {
					auto captures = this->captures.getSelection();
					for (auto capture : captures) {
						ofPushMatrix();
						{
							ofMultMatrix(capture->previewTransform);
							ofPushStyle();
							{
								//draw a little axis to represent recorded pose
								ofSetColor(capture->color);
								ofDrawLine(ofVec3f(), ofVec3f(0.1, 0.0, 0.0));
								ofDrawLine(ofVec3f(), ofVec3f(0.0, 0.1, 0.0));
								ofDrawLine(ofVec3f(), ofVec3f(0.0, 0.0, 0.1));
							}
							ofPopStyle();
						}
						ofPopMatrix();
					}
				}
			}

			//----------
			void MatchMarkers::populateInspector(ofxCvGui::InspectArguments & inspectArgs) {
				auto inspector = inspectArgs.inspector;
				this->captures.populateWidgets(inspector);
				inspector->addButton("Add capture", [this]() {
					this->needsTakeCapture.store(true);
				});
				inspector->addButton("Force use capture", [this]() {
					this->needsForceUseCapture.store(true);
				});
			}

			//----------
			void MatchMarkers::serialize(Json::Value & json) {
				this->captures.serialize(json);
			}

			//----------
			void MatchMarkers::deserialize(const Json::Value & json) {
				this->captures.deserialize(json);
			}

			//----------
			void MatchMarkers::processFrame(shared_ptr<FindMarkerCentroidsFrame> incomingFrame) {
				//construct the output frame
				auto outputFrame = make_shared<MatchMarkersFrame>();
				outputFrame->incomingFrame = incomingFrame;

				{
					auto lock = unique_lock<mutex>(this->descriptionMutex);
					outputFrame->bodyDescription = this->bodyDescription;
					if (!outputFrame->bodyDescription) {
						//we can't calculate the frame without a marker body
						return;
					}
					else if (outputFrame->bodyDescription->markerCount == 0) {
						//we can't calculate with an empty marker body
						return;
					}

					outputFrame->cameraDescription = this->cameraDescription;
					if (!outputFrame->cameraDescription) {
						//we can't calculate the frame without a camera
						return;
					}
				}


				//get the distance threshold
				outputFrame->distanceThresholdSquared = this->parameters.trackingDistanceThreshold.get();
				outputFrame->distanceThresholdSquared *= outputFrame->distanceThresholdSquared;

				//process the normal tracking search
				this->processTrackingSearch(outputFrame);

				//try our pre-recorded poses if we failed tracking
				if (!outputFrame->result.success) {
					auto searchFrame = this->processCheckKnownPoses(outputFrame);
					if (searchFrame) {
						//found a historic pose which works
						outputFrame = searchFrame;
					}
					else {
						//we failed
					}
				}

				if (this->needsTakeCapture.load()) {
					auto capture = make_shared<Capture>();
					cv::Mat modelViewRotationVector;
					cv::Mat modelViewTranslation;
					cv::solvePnP(outputFrame->result.objectSpacePoints
						, outputFrame->result.centroids
						, outputFrame->cameraDescription->cameraMatrix
						, outputFrame->cameraDescription->distortionCoefficients
						, modelViewRotationVector
						, modelViewTranslation);

					capture->modelViewRotationVector = (cv::Point3d)modelViewRotationVector;
					capture->modelViewTranslation = (cv::Point3d)modelViewTranslation;
					capture->previewTransform = ofxCv::makeMatrix(modelViewRotationVector
						, modelViewTranslation).getInverse() * outputFrame->bodyDescription->modelTransform;
					
					this->newCaptures.send(capture);
					this->needsTakeCapture.store(false);
				}

				if (this->needsForceUseCapture.load()) {
					auto captures = this->captures.getSelection();
					if (!captures.empty()) {
						auto capture = * captures.begin();
						outputFrame->modelViewRotationVector = cv::Mat(capture->modelViewRotationVector);
						outputFrame->modelViewTranslation = cv::Mat(capture->modelViewTranslation);
						this->processModelViewTransform(outputFrame);
						outputFrame->result.forceTakeTransform = true;
					}
					this->needsForceUseCapture.store(false);
				}

				this->onNewFrame.notifyListeners(move(outputFrame));
			}

			//----------
			void MatchMarkers::processTrackingSearch(shared_ptr<MatchMarkersFrame> & outputFrame) {
				//combine the camera and body transforms
				cv::composeRT(outputFrame->bodyDescription->rotationVector
					, outputFrame->bodyDescription->translation
					, outputFrame->cameraDescription->inverseRotationVector
					, outputFrame->cameraDescription->inverseTranslation
					, outputFrame->modelViewRotationVector
					, outputFrame->modelViewTranslation);

				//first check the markers are inside the camera image
				//(cvProjectPoints will happily give us weird results for markers outside view, e.g. distortion loop back, behind cam)
				{
					vector<ofVec3f> matrixProjections;
					for (int i = 0; i < bodyDescription->markerCount; i++) {
						const auto & objectSpacePoint = bodyDescription->markers.positions[i];
						const auto worldSpace = objectSpacePoint * bodyDescription->modelTransform;
						const auto projectionSpace = worldSpace * cameraDescription->viewProjectionMatrix;
						if (projectionSpace.z < -1 || projectionSpace.z > 1) {
							//outside of depth clipping range
							continue;
						}
						if (projectionSpace.x < -2.0
							|| projectionSpace.x > 2.0
							|| projectionSpace.y < -2.0
							|| projectionSpace.y > 2.0) {
							//outside of image space (with some allowance for distortion)
							continue;
						}
						outputFrame->search.markerIDs.push_back(bodyDescription->markers.IDs[i]);
						outputFrame->search.objectSpacePoints.emplace_back(move(ofxCv::toCv(objectSpacePoint)));
					}
					outputFrame->search.count = outputFrame->search.markerIDs.size();
				}

				this->processModelViewTransform(outputFrame);
			}

			//----------
			shared_ptr<MatchMarkersFrame> MatchMarkers::processCheckKnownPoses(shared_ptr<MatchMarkersFrame> & outputFrame) {
				auto captures = this->captures.getSelection();
				for (auto capture : captures) {
					auto searchFrame = make_shared<MatchMarkersFrame>(* outputFrame);

					searchFrame->modelViewRotationVector = cv::Mat(capture->modelViewRotationVector);
					searchFrame->modelViewTranslation = cv::Mat(capture->modelViewTranslation);
					
					searchFrame->distanceThresholdSquared = this->parameters.refindTrackingThreshold;
					searchFrame->distanceThresholdSquared *= searchFrame->distanceThresholdSquared;

					this->processModelViewTransform(searchFrame);
					capture->reprojectionError = searchFrame->result.reprojectionError;

					if (searchFrame->result.success) {
						//now check it with the tracking distance threshold
						searchFrame->distanceThresholdSquared = this->parameters.trackingDistanceThreshold;
						this->processModelViewTransform(searchFrame);

						if (searchFrame->result.success) {
							//great we passed the test!

							//mark that we've jumped to somewhere new
							searchFrame->result.trackingWasLost = true;
							
							return searchFrame;
						}
					}
				}

				//we failed
				return nullptr;
			}

			//----------
			void MatchMarkers::processModelViewTransform(shared_ptr<MatchMarkersFrame> & outputFrame) {
				if (outputFrame->search.objectSpacePoints.empty()) {
					return;
				}

				//project all 3D points from marker body (based on existing/predicted pose) into camera space
				cv::projectPoints(outputFrame->search.objectSpacePoints
					, outputFrame->modelViewRotationVector
					, outputFrame->modelViewTranslation
					, outputFrame->cameraDescription->cameraMatrix
					, outputFrame->cameraDescription->distortionCoefficients
					, outputFrame->search.projectedMarkerImagePoints);

				//clear the result
				outputFrame->result = MatchMarkersFrame::Result();

				for (size_t centroidIndex = 0; centroidIndex < outputFrame->incomingFrame->centroids.size(); centroidIndex++) {
					const auto & centroid = outputFrame->incomingFrame->centroids[centroidIndex];

					map<float, size_t> matchesByDistance; // { distanceSquared, marker index in incoming frame vector}

														  //find all matching markers
					for (size_t i = 0; i < outputFrame->search.count; i++) {
						//find distance
						const auto & markerProjected = outputFrame->search.projectedMarkerImagePoints[i];
						const auto delta = centroid - markerProjected;
						const auto distanceSquared = delta.x * delta.x + delta.y * delta.y;

						if (distanceSquared < outputFrame->distanceThresholdSquared) {
							matchesByDistance.emplace(distanceSquared, i);
						}
					}

					if (matchesByDistance.empty()) {
						//no match found
						continue;
					}

					auto matchIndex = matchesByDistance.begin()->second;
					outputFrame->result.markerListIndicies.push_back(matchIndex);
					outputFrame->result.markerIDs.push_back(outputFrame->search.markerIDs[matchIndex]);
					outputFrame->result.projectedPoints.push_back(outputFrame->search.projectedMarkerImagePoints[matchIndex]);
					outputFrame->result.centroids.push_back(centroid);
					outputFrame->result.centroidIndex.push_back(centroidIndex);
					outputFrame->result.objectSpacePoints.push_back(outputFrame->search.objectSpacePoints[matchIndex]);
				}

				outputFrame->result.count = outputFrame->result.markerIDs.size();
				outputFrame->result.success = outputFrame->result.count >= 4;

				if (outputFrame->result.success) {
					//calculate reprojection error
					float sumErrorsSquared = 0.0f;
					for (int i = 0; i < outputFrame->result.count; i++) {
						auto delta = outputFrame->result.centroids[i] - outputFrame->result.projectedPoints[i];
						sumErrorsSquared += delta.dot(delta);
					}
					outputFrame->result.reprojectionError = sqrt(sumErrorsSquared);
				}
			}

		}
	}
}
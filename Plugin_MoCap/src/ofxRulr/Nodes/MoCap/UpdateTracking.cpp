	#include "pch_Plugin_MoCap.h"
#include "UpdateTracking.h"

#include "Body.h"

namespace ofxRulr {
	namespace Nodes {
		namespace MoCap {

			//----------
			UpdateTracking::UpdateTracking() {
				RULR_NODE_INIT_LISTENER;
			}

			//----------
			std::string UpdateTracking::getTypeName() const {
				return "MoCap::UpdateTracking";
			}

			//----------
			void UpdateTracking::init() {
				RULR_NODE_UPDATE_LISTENER;

				this->addInput<Body>();
			}

			//----------
			void UpdateTracking::update() {
				{
					auto lock = unique_lock<mutex>(bodyNodeMutex);
					this->bodyNode = this->getInput<Body>();
				}
			}

			//----------
			void UpdateTracking::processFrame(shared_ptr<MatchMarkersFrame> incomingFrame) {
				//ignore if less than 3
				if (incomingFrame->matchCount < 4) {
					return;
				}

				//get the body to update
				this->bodyNodeMutex.lock();
				auto bodyNode = this->bodyNode;
				this->bodyNodeMutex.unlock();
				if (!bodyNode) {
					return;
				}
				
				//construct output
				auto outgoingFrame = make_shared<UpdateTrackingFrame>();
				outgoingFrame->incomingFrame = incomingFrame;
				outgoingFrame->modelViewRotationVector = incomingFrame->modelViewRotationVector;
				outgoingFrame->modelViewTranslation = incomingFrame->modelViewTranslation;
				
				cv::solvePnP(incomingFrame->matchedObjectSpacePoints
					, incomingFrame->matchedCentroids
					, incomingFrame->cameraDescription->cameraMatrix
					, incomingFrame->cameraDescription->distortionCoefficients
					, outgoingFrame->modelViewRotationVector
					, outgoingFrame->modelViewTranslation
					, true);

				//apply the inverse of the camera transform
				{
					auto cameraTransform = ofxCv::makeMatrix(incomingFrame->cameraDescription->rotationVector, incomingFrame->cameraDescription->translation);

					cv::Mat cameraRotationInverse;
					cv::Mat cameraTranslationInverse;
					ofxCv::decomposeMatrix(cameraTransform.getInverse()
						, cameraRotationInverse
						, cameraTranslationInverse);

					cv::composeRT(outgoingFrame->modelViewRotationVector
						, outgoingFrame->modelViewTranslation
						, cameraRotationInverse
						, cameraTranslationInverse
						, outgoingFrame->modelRotationVector
						, outgoingFrame->modelTranslation);
				}

				outgoingFrame->transform = bodyNode->updateTracking(outgoingFrame->modelRotationVector
					, outgoingFrame->modelTranslation
					, incomingFrame->trackingWasLost);

				this->onNewFrame.notifyListeners(outgoingFrame);
			}
		}
	}
}
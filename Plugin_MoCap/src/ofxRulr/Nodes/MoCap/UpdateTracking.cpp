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

				this->addInput<Item::RigidBody>();

				this->manageParameters(this->parameters);
			}

			//----------
			void UpdateTracking::update() {
				{
					auto rigidBodyNode = this->getInput<Item::RigidBody>();
					if (rigidBodyNode) {
						shared_ptr<UpdateTrackingFrame> updateTrackingFrame;
						while (this->trackingUpdateToMainThread.tryReceive(updateTrackingFrame)) {}

						if (updateTrackingFrame) {
							rigidBodyNode->setTransform(updateTrackingFrame->transform);
						}
					}
				}
			}

			//----------
			void UpdateTracking::processFrame(shared_ptr<MatchMarkersFrame> incomingFrame) {
				//ignore if less than 3
				if (incomingFrame->result.count < 4) {
					return;
				}
				
				//construct output
				auto outgoingFrame = make_shared<UpdateTrackingFrame>();
				outgoingFrame->incomingFrame = incomingFrame;
				outgoingFrame->updateTarget = this->parameters.updateTarget;
				outgoingFrame->bodyModelViewRotationVector = incomingFrame->modelViewRotationVector;
				outgoingFrame->bodyModelViewTranslation = incomingFrame->modelViewTranslation;
				
				cv::solvePnP(incomingFrame->result.objectSpacePoints
					, incomingFrame->result.centroids
					, incomingFrame->cameraDescription->cameraMatrix
					, incomingFrame->cameraDescription->distortionCoefficients
					, outgoingFrame->bodyModelViewRotationVector
					, outgoingFrame->bodyModelViewTranslation
					, true);

				//apply the inverse of the camera transform
				{
					auto cameraTransform = ofxCv::makeMatrix(incomingFrame->cameraDescription->rotationVector, incomingFrame->cameraDescription->translation);

					cv::Mat cameraRotationInverse;
					cv::Mat cameraTranslationInverse;
					ofxCv::decomposeMatrix(cameraTransform.getInverse()
						, cameraRotationInverse
						, cameraTranslationInverse);

					cv::composeRT(outgoingFrame->bodyModelViewRotationVector
						, outgoingFrame->bodyModelViewTranslation
						, cameraRotationInverse
						, cameraTranslationInverse
						, outgoingFrame->modelRotationVector
						, outgoingFrame->modelTranslation);
				}

				switch (outgoingFrame->updateTarget.get()) {
					case UpdateTarget::Body:
					{
						//nothing to do
						outgoingFrame->transform = ofxCv::makeMatrix(outgoingFrame->modelRotationVector
							, outgoingFrame->modelTranslation);
						break;
					}
					case UpdateTarget::Camera:
					{
						//special case, only the transform is in camera coords
						outgoingFrame->transform = ofxCv::makeMatrix(outgoingFrame->bodyModelViewRotationVector
							, outgoingFrame->bodyModelViewTranslation).getInverse() * incomingFrame->bodyDescription->modelTransform;
						break;
					}
					default:
						break;
				}

				this->onNewFrame.notifyListeners(outgoingFrame);
				this->trackingUpdateToMainThread.send(outgoingFrame);
			}
		}
	}
}
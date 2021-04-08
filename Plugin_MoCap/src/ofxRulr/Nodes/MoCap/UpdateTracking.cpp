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
				RULR_NODE_INSPECTOR_LISTENER;

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
			void UpdateTracking::populateInspector(ofxCvGui::InspectArguments & inspectArgs) {
				auto insepctor = inspectArgs.inspector;
				insepctor->addLiveValue<float>("Reprojection error [px]", [this]() {
					return this->reprojectionError.load();
				});
			}

			//----------
			void UpdateTracking::processFrame(shared_ptr<MatchMarkersFrame> incomingFrame) {
				//ignore if less than 3
				if (!(incomingFrame->result.success || incomingFrame->result.forceTakeTransform)) {
					//do nothing
					return;
				}
				
				//construct output
				auto outgoingFrame = make_shared<UpdateTrackingFrame>();
				outgoingFrame->incomingFrame = incomingFrame;
				outgoingFrame->updateTarget = this->parameters.updateTarget;
				outgoingFrame->bodyModelViewRotationVector = incomingFrame->modelViewRotationVector;
				outgoingFrame->bodyModelViewTranslation = incomingFrame->modelViewTranslation;
				
				if (incomingFrame->result.forceTakeTransform) {
					outgoingFrame->bodyModelViewRotationVector = incomingFrame->modelViewRotationVector;
					outgoingFrame->bodyModelViewTranslation = incomingFrame->modelViewTranslation;
				}
				else {
					cv::solvePnP(incomingFrame->result.objectSpacePoints
						, incomingFrame->result.centroids
						, incomingFrame->cameraDescription->cameraMatrix
						, incomingFrame->cameraDescription->distortionCoefficients
						, outgoingFrame->bodyModelViewRotationVector
						, outgoingFrame->bodyModelViewTranslation
						, true);
				}

				//ignore if reprojection error is too high
				{
					cv::projectPoints(incomingFrame->result.objectSpacePoints
						, outgoingFrame->bodyModelViewRotationVector
						, outgoingFrame->bodyModelViewTranslation
						, incomingFrame->cameraDescription->cameraMatrix
						, incomingFrame->cameraDescription->distortionCoefficients
						, outgoingFrame->reprojectedAfterTracking);

					float sumErrorSquared = 0.0f;
					for (int i = 0; i < incomingFrame->result.count; i++) {
						const auto delta = outgoingFrame->reprojectedAfterTracking[i] - incomingFrame->result.centroids[i];
						sumErrorSquared += delta.dot(delta);
					}
					auto reprojectionError = sqrt(sumErrorSquared);
					this->reprojectionError.store(reprojectionError);
					auto reprojectionThreshold = this->parameters.reprojectionThreshold.get();
					if (reprojectionError > reprojectionThreshold && !incomingFrame->result.forceTakeTransform) {
						//reprojection error is too high
						return;
					}
				}

				//apply the inverse of the camera transform
				{
					auto cameraTransform = ofxCv::makeMatrix(incomingFrame->cameraDescription->inverseRotationVector, incomingFrame->cameraDescription->inverseTranslation);

					cv::Mat cameraRotationInverse;
					cv::Mat cameraTranslationInverse;
					ofxCv::decomposeMatrix(glm::inverse(cameraTransform)
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
						outgoingFrame->transform = glm::inverse(ofxCv::makeMatrix(outgoingFrame->bodyModelViewRotationVector
							, outgoingFrame->bodyModelViewTranslation)) * incomingFrame->bodyDescription->modelTransform;
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
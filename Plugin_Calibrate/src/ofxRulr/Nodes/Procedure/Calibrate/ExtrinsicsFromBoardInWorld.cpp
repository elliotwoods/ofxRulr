#include "pch_Plugin_Calibrate.h"
#include "ExtrinsicsFromBoardInWorld.h"

#include "ofxRulr/Nodes/Item/Camera.h"
#include "ofxRulr/Nodes/Item/BoardInWorld.h"

namespace ofxRulr {
	namespace Nodes {
		namespace Procedure {
			namespace Calibrate {
				//----------
				ExtrinsicsFromBoardInWorld::ExtrinsicsFromBoardInWorld() {
					RULR_NODE_INIT_LISTENER;
				}

				//----------
				void ExtrinsicsFromBoardInWorld::init() {
					RULR_NODE_UPDATE_LISTENER;
					RULR_NODE_SERIALIZATION_LISTENERS;
					RULR_NODE_INSPECTOR_LISTENER;
					RULR_NODE_DRAW_WORLD_LISTENER;

					this->addInput<Item::Camera>();
					this->addInput<Item::BoardInWorld>();

					{
						auto view = ofxCvGui::Panels::makeImage(this->image);
						view->onDrawImage += [this](ofxCvGui::DrawImageArguments& drawArgs) {
							ofxCv::drawCorners(this->imagePoints);
						};
						this->view = view;
					}
				}

				//----------
				string ExtrinsicsFromBoardInWorld::getTypeName() const {
					return "Procedure::Calibrate::ExtrinsicsFromBoardInWorld";
				}

				//----------
				ofxCvGui::PanelPtr ExtrinsicsFromBoardInWorld::getPanel() {
					return this->view;
				}

				//----------
				void ExtrinsicsFromBoardInWorld::update() {
					// tethered shooting
					{
						bool tetheredEnabled = false;
						switch (this->parameters.capture.tetheredShootEnabled.get()) {
						case WhenDrawOnWorldStage::Selected:
							tetheredEnabled = this->isBeingInspected();
							break;
						case WhenDrawOnWorldStage::Always:
							tetheredEnabled = true;
						default:
							break;
						}
						if (tetheredEnabled) {
							auto camera = this->getInput<Item::Camera>();
							if (camera) {
								auto grabber = camera->getGrabber();
								if (grabber) {
									if (grabber->isFrameNew()
										&& !grabber->getDeviceSpecification().supports(ofxMachineVision::CaptureSequenceType::Continuous)
										&& grabber->getDeviceSpecification().supports(ofxMachineVision::CaptureSequenceType::OneShot)) {
										try {
											Utils::ScopedProcess scopedProcess("Tethered capture");
											this->track(this->parameters.track.updateTarget.get(), false);
											scopedProcess.end();
										}
										RULR_CATCH_ALL_TO_ALERT
									}
								}
							}
						}
					}
				}

				//----------
				void ExtrinsicsFromBoardInWorld::drawWorldStage() {
					auto camera = this->getInput<Item::Camera>();
					if (camera) {
						ofPushMatrix();
						{
							ofMultMatrix(camera->getTransform());

						}
						ofPopMatrix();
					}
				}

				//----------
				void ExtrinsicsFromBoardInWorld::serialize(nlohmann::json& json) {
					Utils::serialize(json, this->parameters);
				}

				//----------
				void ExtrinsicsFromBoardInWorld::deserialize(const nlohmann::json& json) {
					Utils::deserialize(json, this->parameters);
				}

				//----------
				void ExtrinsicsFromBoardInWorld::populateInspector(ofxCvGui::InspectArguments& inspectArguments) {
					auto inspector = inspectArguments.inspector;

					inspector->addButton("Track fresh frame", [this]() {
						try {
							Utils::ScopedProcess scopedProcess("Track fresh frame");
							this->track(this->parameters.track.updateTarget.get(), true);
							scopedProcess.end();
						}
						RULR_CATCH_ALL_TO_ERROR;
						}
					, ' ');

					inspector->addButton("Track prior frame", [this]() {
						try {
							Utils::ScopedProcess scopedProcess("Track prior frame");
							this->track(this->parameters.track.updateTarget.get(), false);
							scopedProcess.end();
						}
						RULR_CATCH_ALL_TO_ERROR;
						}
					, OF_KEY_RETURN)->setHeight(100.0f);

					inspector->addParameterGroup(this->parameters);
				}

				//----------
				void ExtrinsicsFromBoardInWorld::track(const UpdateTarget& updateTarget, bool getFreshFrame) {
					this->throwIfMissingAnyConnection();
					auto camera = this->getInput<Item::Camera>();
					auto boardInWorld = this->getInput<Item::BoardInWorld>();
					boardInWorld->throwIfMissingAnyConnection();

					shared_ptr<ofxMachineVision::Frame> frame;
					{
						auto grabber = camera->getGrabber();
						if (!grabber) {
							throw(ofxRulr::Exception("No grabber found"));
						}
						if (getFreshFrame) {
							Utils::ScopedProcess scopedProcess("Get fresh frame");
							frame = grabber->getFreshFrame();
							scopedProcess.end();
						}
						else {
							frame = grabber->getFrame();
						}
					}

					if (!frame) {
						throw(Exception("Frame is not valid"));
					}
					auto& pixels = frame->getPixels();
					if (pixels.size() == 0) {
						throw(Exception("Pixels are empty"));
					}

					auto image = ofxCv::toCv(frame->getPixels());
					this->track(updateTarget, image);
				}

				//----------
				void ExtrinsicsFromBoardInWorld::track(const UpdateTarget& updateTarget, const cv::Mat& image) {
					this->throwIfMissingAConnection<Item::Camera>();
					this->throwIfMissingAConnection<Item::BoardInWorld>();

					this->image.allocate(image.cols
						, image.rows
						, ofImageType::OF_IMAGE_GRAYSCALE);

					if (image.channels() == 1) {
						memcpy(this->image.getPixels().getData()
							, image.data
							, this->image.getPixels().size());
					}
					else {
						cv::cvtColor(image
							, ofxCv::toCv(this->image)
							, cv::COLOR_RGB2GRAY);
					}
					this->image.update();

					auto boardInWorld = this->getInput<Item::BoardInWorld>();
					auto camera = this->getInput<Item::Camera>();

					// find the board
					{
						Utils::ScopedProcess scopedProcess("Find board in image");
						if (!boardInWorld->findBoard(ofxCv::toCv(this->image)
							, this->imagePoints
							, this->objectPoints
							, this->worldPoints
							, this->parameters.capture.findBoardMode
							, camera->getCameraMatrix()
							, camera->getDistortionCoefficients())) {
							throw(ofxRulr::Exception("Board not found in image"));
						}
						scopedProcess.end();
					}
					

					// Solve PnP
					cv::Mat boardToCameraRotationVector, boardToCameraTranslation;
					{
						// We are quite confident that we're naming all the rotations/translation correctly here
						// Note that when we set extrinsics, we inverse because Extrinsics = from the objects reference plane to the world reference frame
						Utils::ScopedProcess solvePnPScope("solvePnP");
						const auto& useExtrinsicGuess = this->parameters.track.useExtrinsicGuess.get();
						if (useExtrinsicGuess) {
							camera->getExtrinsics(boardToCameraRotationVector
								, boardToCameraTranslation
								, true);
						}
						cv::solvePnP(this->objectPoints
							, this->imagePoints
							, camera->getCameraMatrix()
							, camera->getDistortionCoefficients()
							, boardToCameraRotationVector
							, boardToCameraTranslation
							, useExtrinsicGuess);

						solvePnPScope.end();
					}

					// Apply transform to appropriate target
					if (updateTarget == UpdateTarget::Camera) {
						Utils::ScopedProcess updateTargetScope("Update camera");

						cv::Mat worldToBodyRotationVector, worldToBodyTranslation;
						boardInWorld->getExtrinsics(worldToBodyRotationVector
							, worldToBodyTranslation
							, true);

						cv::Mat worldToCameraRotationVector, worldToCameraTranslation;
						cv::composeRT(
							worldToBodyRotationVector
							, worldToBodyTranslation
							, boardToCameraRotationVector
							, boardToCameraTranslation
							, worldToCameraRotationVector
							, worldToCameraTranslation);

						camera->setExtrinsics(worldToCameraRotationVector
							, worldToCameraTranslation
							, true);

						updateTargetScope.end();
					}
					else {
						Utils::ScopedProcess updateTargetScope("Update board");

						// For this side (board update) we just the the transforms.
						// We tried with composeRT but failed

						auto boardInCameraTransform = ofxCv::makeMatrix(boardToCameraRotationVector
							, boardToCameraTranslation);
						auto boardInWorldTransform = camera->getTransform() * boardInCameraTransform;
						boardInWorld->setTransform(boardInWorldTransform);

						updateTargetScope.end();
					}
				}
			}
		}
	}
}

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
											this->receiveFrame(grabber->getFrame());
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

					{
						auto grabber = camera->getGrabber();
						if (!grabber) {
							throw(ofxRulr::Exception("No grabber found"));
						}
						if (getFreshFrame) {
							Utils::ScopedProcess scopedProcess("Get fresh frame");
							auto frame = grabber->getFreshFrame();
							this->receiveFrame(frame);
							scopedProcess.end();
						}
						else {
							auto frame = grabber->getFrame();
							if (!frame) {
								throw(ofxRulr::Exception("No prior frame available"));
							}
							this->receiveFrame(frame);
						}
					}

					{
						Utils::ScopedProcess scopedProcess("Track");

						{
							Utils::ScopedProcess findBoardScope("findBoard");
							auto board = boardInWorld->getInput<Item::AbstractBoard>();
							auto foundBoard = board->findBoard(ofxCv::toCv(this->image.getPixels())
								, this->imagePoints
								, this->objectPoints
								, this->parameters.capture.findBoardMode.get()
								, camera->getCameraMatrix()
								, camera->getDistortionCoefficients());
							if (!foundBoard) {
								throw(Exception("Board not found"));
							}
							findBoardScope.end();
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

							cv::Mat worldToCameraRotationVector, worldToCameraTranslation;
							camera->getExtrinsics(worldToCameraRotationVector
								, worldToCameraTranslation
								, true);

							// invert the extrinsics
							cv::Mat cameraToBoardRotationVector, cameraToBoardTranslation;
							{
								auto matrix = ofxCv::makeMatrix(boardToCameraRotationVector
									, boardToCameraTranslation);
								auto inverseMatrix = glm::inverse(matrix);
								ofxCv::decomposeMatrix(matrix, cameraToBoardRotationVector, cameraToBoardTranslation);
							}

							cv::Mat worldToBoardRotationVector, worldToBoardTranslation;
							cv::composeRT(
								worldToCameraRotationVector
								, worldToCameraTranslation
								, cameraToBoardRotationVector
								, cameraToBoardTranslation
								, worldToBoardRotationVector
								, worldToBoardTranslation);

							boardInWorld->setExtrinsics(worldToBoardRotationVector
								, worldToBoardTranslation
								, true);

							updateTargetScope.end();
						}

						scopedProcess.end();
					}
				}

				//----------
				void ExtrinsicsFromBoardInWorld::receiveFrame(shared_ptr<ofxMachineVision::Frame> frame) {
					this->throwIfMissingAConnection<Item::Camera>();
					this->throwIfMissingAConnection<Item::BoardInWorld>();

					if (!frame) {
						throw(Exception("Frame is not valid"));
					}
					auto& pixels = frame->getPixels();
					if (pixels.size() == 0) {
						throw(Exception("Pixels are empty"));
					}

					if (pixels.getNumChannels() == 1) {
						this->image.getPixels() = pixels;
					}
					else {
						this->image.allocate(pixels.getWidth()
							, pixels.getHeight()
							, ofImageType::OF_IMAGE_GRAYSCALE);
						cv::cvtColor(ofxCv::toCv(pixels)
							, ofxCv::toCv(this->image)
							, cv::COLOR_RGB2GRAY);
					}
					this->image.update();

					auto boardInWorld = this->getInput<Item::BoardInWorld>();
					auto camera = this->getInput<Item::Camera>();

					if (!boardInWorld->findBoard(ofxCv::toCv(this->image)
						, this->imagePoints
						, this->objectPoints
						, this->worldPoints
						, this->parameters.capture.findBoardMode
						, camera->getCameraMatrix()
						, camera->getDistortionCoefficients())) {
						throw(ofxRulr::Exception("Board not found in image"));
					}
				}
			}
		}
	}
}

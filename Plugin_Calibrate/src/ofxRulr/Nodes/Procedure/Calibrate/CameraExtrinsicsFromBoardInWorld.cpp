#include "pch_Plugin_Calibrate.h"
#include "CameraExtrinsicsFromBoardInWorld.h"

#include "ofxRulr/Nodes/Item/Camera.h"
#include "ofxRulr/Nodes/Item/BoardInWorld.h"

namespace ofxRulr {
	namespace Nodes {
		namespace Procedure {
			namespace Calibrate {
				//----------
				CameraExtrinsicsFromBoardInWorld::CameraExtrinsicsFromBoardInWorld() {
					RULR_NODE_INIT_LISTENER;
				}

				//----------
				void CameraExtrinsicsFromBoardInWorld::init() {
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
				string CameraExtrinsicsFromBoardInWorld::getTypeName() const {
					return "Procedure::Calibrate::CameraExtrinsicsFromBoardInWorld";
				}

				//----------
				ofxCvGui::PanelPtr CameraExtrinsicsFromBoardInWorld::getPanel() {
					return this->view;
				}

				//----------
				void CameraExtrinsicsFromBoardInWorld::update() {
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
				void CameraExtrinsicsFromBoardInWorld::drawWorldStage() {
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
				void CameraExtrinsicsFromBoardInWorld::serialize(nlohmann::json& json) {
					Utils::serialize(json, this->parameters);
				}

				//----------
				void CameraExtrinsicsFromBoardInWorld::deserialize(const nlohmann::json& json) {
					Utils::deserialize(json, this->parameters);
				}

				//----------
				void CameraExtrinsicsFromBoardInWorld::populateInspector(ofxCvGui::InspectArguments& inspectArguments) {
					auto inspector = inspectArguments.inspector;

					inspector->addButton("Capture", [this]() {
						try {
							this->capture();
						}
						RULR_CATCH_ALL_TO_ERROR;
						}
					, ' ');

					inspector->addButton("Calibrate", [this]() {
						try {
							this->calibrate();
						}
						RULR_CATCH_ALL_TO_ERROR;
						}
					, OF_KEY_RETURN)->setHeight(100.0f);

					inspector->addParameterGroup(this->parameters);
				}

				//----------
				void CameraExtrinsicsFromBoardInWorld::capture() {
					Utils::ScopedProcess scopedProcess("Capture");

					this->throwIfMissingAConnection<Item::Camera>();
					auto camera = this->getInput<Item::Camera>();

					auto grabber = camera->getGrabber();
					if (!grabber) {
						throw(Exception("Couldn't get grabber"));
					}
					auto frame = grabber->getFreshFrame();
					this->receiveFrame(frame);

					if (this->parameters.calibrate.calibrateOnCapture) {
						this->calibrate();
					}
					scopedProcess.end();
				}

				//----------
				void CameraExtrinsicsFromBoardInWorld::calibrate() {
					Utils::ScopedProcess scopedProcess("Calibrate");

					this->throwIfMissingAnyConnection();

					auto camera = this->getInput<Item::Camera>();
					auto boardInWorld = this->getInput<Item::BoardInWorld>();
					boardInWorld->throwIfMissingAnyConnection();

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

					{
						// We are quite confident that we're naming all the rotations/translation correctly here
						// Note that when we set extrinsics, we inverse because Extrinsics = from the objects reference plane to the world reference frame
						Utils::ScopedProcess solvePnPScope("solvePnP");
						cv::Mat bodyToCameraRotationVector, bodyToCameraTranslation;
						const auto & useExtrinsicGuess = this->parameters.calibrate.useExtrinsicGuess.get();
						if (useExtrinsicGuess) {
							camera->getExtrinsics(bodyToCameraRotationVector
								, bodyToCameraTranslation
								, true);
						}
						cv::solvePnP(this->objectPoints
							, this->imagePoints
							, camera->getCameraMatrix()
							, camera->getDistortionCoefficients()
							, bodyToCameraRotationVector
							, bodyToCameraTranslation
							, useExtrinsicGuess);

						cv::Mat worldToBodyRotationVector, worldToBodyTranslation;
						boardInWorld->getExtrinsics(worldToBodyRotationVector
							, worldToBodyTranslation
							, true);

						cv::Mat worldToCameraRotationVector, worldToCameraTranslation;
						cv::composeRT(
							worldToBodyRotationVector
							, worldToBodyTranslation
							, bodyToCameraRotationVector
							, bodyToCameraTranslation
							, worldToCameraRotationVector
							, worldToCameraTranslation);

						camera->setExtrinsics(worldToCameraRotationVector
							, worldToCameraTranslation
							, true);
						solvePnPScope.end();
					}

					scopedProcess.end();
				}

				//----------
				void CameraExtrinsicsFromBoardInWorld::receiveFrame(shared_ptr<ofxMachineVision::Frame> frame) {
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
				}
			}
		}
	}
}

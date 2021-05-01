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

					inspector->addButton("Capture and calibrate", [this]() {
						try {
							this->captureAndCalibrate();
						}
						RULR_CATCH_ALL_TO_ERROR;
						}
					, OF_KEY_RETURN)->setHeight(100.0f);
					inspector->addParameterGroup(this->parameters);
				}

				//----------
				void CameraExtrinsicsFromBoardInWorld::captureAndCalibrate() {
					this->throwIfMissingAnyConnection();
					auto camera = this->getInput<Item::Camera>();
					auto boardInWorld = this->getInput<Item::BoardInWorld>();
					boardInWorld->throwIfMissingAnyConnection();

					Utils::ScopedProcess captureAndCalibrateScope("captureAndCalibrate");

					{
						Utils::ScopedProcess getImageScope("getFreshFrame");
						auto grabber = camera->getGrabber();
						if (!grabber) {
							throw(Exception("Couldn't get grabber"));
						}
						auto frame = grabber->getFreshFrame();
						if (!frame) {
							throw(Exception("Failed to get fresh frame from grabber"));
						}

						auto& pixels = frame->getPixels();
						if (pixels.size() == 0) {
							throw(Exception("Pixels are empty"));
						}

						if (pixels.getNumChannels() == 1) {
							this->image.getPixels() = pixels;
						}
						else {
							cv::cvtColor(ofxCv::toCv(pixels)
								, ofxCv::toCv(this->image)
								, cv::COLOR_RGB2GRAY);
						}
						this->image.update();
					}

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
					}
				}
			}
		}
	}
}

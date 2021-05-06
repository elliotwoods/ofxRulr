#include "pch_Plugin_Experiments.h"

namespace ofxRulr {
	namespace Nodes {
		namespace Experiments {
			namespace MirrorPlaneCapture {
#pragma mark PhotoCapture

				//--------
				BoardInMirror2::PhotoCapture::PhotoCapture() {
					RULR_SERIALIZE_LISTENERS;
				}

				//--------
				void BoardInMirror2::PhotoCapture::drawWorld() {
					ofPushStyle();
					{
						ofSetColor(this->color);
						ofSetSphereResolution(6);
						for (const auto& worldPoint : this->worldPoints) {
							ofDrawSphere(ofxCv::toOf(worldPoint), 0.01);
						}
					}
					ofPopStyle();

					for (const auto& ray : this->cameraRays) {
						ray.draw();
					}
				}

				//--------
				string BoardInMirror2::PhotoCapture::getDisplayString() const {
					stringstream ss;
					ss << this->objectPoints.size() << " points found.";
					return ss.str();
				}

				//--------
				void BoardInMirror2::PhotoCapture::serialize(nlohmann::json& json) {
					Utils::serialize(json["imagePoints"], this->imagePoints);
					Utils::serialize(json["objectPoints"], this->objectPoints);
					Utils::serialize(json["worldPoints"], this->worldPoints);
					Utils::serialize(json["cameraRays"], this->cameraRays);

					{
						auto& jsonCameraNavigation = json["cameraNavigation"];
						Utils::serialize(jsonCameraNavigation["imagePoints"], this->cameraNavigation.imagePoints);
						Utils::serialize(jsonCameraNavigation["worldPoints"], this->cameraNavigation.worldPoints);
						Utils::serialize(jsonCameraNavigation["reprojectionError"], this->cameraNavigation.reprojectionError);
						Utils::serialize(jsonCameraNavigation["cameraPosition"], this->cameraNavigation.cameraPosition);
					}
				}

				//--------
				void BoardInMirror2::PhotoCapture::deserialize(const nlohmann::json& json) {
					Utils::deserialize(json, "imagePoints", this->imagePoints);
					Utils::deserialize(json, "objectPoints", this->objectPoints);
					Utils::deserialize(json, "worldPoints", this->worldPoints);
					Utils::deserialize(json, "cameraRays", this->cameraRays);

					{
						auto& jsonCameraNavigation = json["cameraNavigation"];
						Utils::deserialize(jsonCameraNavigation, "imagePoints", this->cameraNavigation.imagePoints);
						Utils::deserialize(jsonCameraNavigation, "worldPoints", this->cameraNavigation.worldPoints);
						Utils::deserialize(jsonCameraNavigation, "reprojectionError", this->cameraNavigation.reprojectionError);
						Utils::deserialize(jsonCameraNavigation, "cameraPosition", this->cameraNavigation.cameraPosition);
					}
				}

#pragma mark BoardInMirror2

				//--------
				BoardInMirror2::BoardInMirror2() {
					RULR_NODE_INIT_LISTENER;
				}

				//--------
				string BoardInMirror2::getTypeName() const {
					return "Experiments::MirrorPlaneCapture::BoardInMirror2";
				}

				//--------
				void BoardInMirror2::init() {
					RULR_NODE_UPDATE_LISTENER;
					RULR_NODE_SERIALIZATION_LISTENERS;
					RULR_NODE_INSPECTOR_LISTENER;

					this->manageParameters(this->parameters);

					this->addInput<Heliostats>();
					this->addInput<Item::Camera>();
					this->addInput<Item::BoardInWorld>();
					this->addInput<ArUco::MarkerMap>();
					this->addInput<ArUco::Detector>();
				}

				//--------
				void BoardInMirror2::update() {
					{
						bool tetheredEnabled = false;
						switch (this->parameters.tetheredShootEnabled.get()) {
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
								if (camera->isNewSingleShotFrame()) {
									try {
										Utils::ScopedProcess scopedProcess("Tethered capture");
										this->addCapture(camera->getFrame());
										scopedProcess.end();
									}
									RULR_CATCH_ALL_TO_ERROR
								}
							}
						}
					}


				}

				//--------
				void BoardInMirror2::capture() {
					Utils::ScopedProcess scopedProcess("Capture");
					this->throwIfMissingAConnection<Item::Camera>();
					auto camera = this->getInput<Item::Camera>();
					auto frame = camera->getFreshFrame();
					this->addCapture(frame);
					scopedProcess.end();
				}

				//--------
				void BoardInMirror2::calibrate() {

				}

				//--------
				ofxCvGui::PanelPtr BoardInMirror2::getPanel() {
					return this->panel;
				}

				//--------
				void BoardInMirror2::populateInspector(ofxCvGui::InspectArguments& inspectArgs) {
					auto inspector = inspectArgs.inspector;
					this->captures.populateWidgets(inspector);
					inspector->addButton("Capture", [this]() {
						try {
							this->capture();
						}
						RULR_CATCH_ALL_TO_ERROR
						}, ' ');
					inspector->addButton("Calibrate", [this]() {
						try {
							this->calibrate();
						}
						RULR_CATCH_ALL_TO_ERROR
						}, OF_KEY_RETURN)->setHeight(100.0f);
				}

				//--------
				void BoardInMirror2::serialize(nlohmann::json& json) {
					this->captures.serialize(json["captures"]);
				}

				//--------
				void BoardInMirror2::deserialize(const nlohmann::json& json) {
					if (json.contains("captures")) {
						this->captures.deserialize(json["captures"]);
					}
				}

				//--------
				void BoardInMirror2::drawWorldStage() {
					auto captures = this->captures.getSelection();
					for (const auto& capture : captures) {
						capture->drawWorld();
					}
				}

				//--------
				void BoardInMirror2::addCapture(shared_ptr<ofxMachineVision::Frame> frame) {
					if (!frame) {
						throw(Exception("Frame is empty"));
					}

					// Make a blank capture
					auto capture = make_shared<PhotoCapture>();

					// Extract the image
					auto image = ofxCv::toCv(frame->getPixels()).clone();
					{
						if (image.empty()) {
							throw("No image in capture");
						}

						// Convert the image if required
						if (image.channels() == 3) {
							cv::cvtColor(image, image, cv::COLOR_RGB2GRAY);
						}
						if (image.type() != CV_8U) {
							image.convertTo(image
								, CV_8U
								, (double)std::numeric_limits<uint8_t>::max() / (double)std::numeric_limits<uint16_t>::max());
						}
					}

					// Get inputs
					this->throwIfMissingAConnection<Item::Camera>();
					this->throwIfMissingAConnection<Item::BoardInWorld>();
					auto camera = this->getInput<Item::Camera>();
					auto boardInWorld = this->getInput<Item::BoardInWorld>();

					auto flipImageMode = this->parameters.findBoard.flipImage.get();

					// Find the board in the reflection
					{
						// Flip the image
						cv::Mat mirroredImage;
						if (flipImageMode != FlipImage::None) {
							cv::Mat mirroredImage;
							cv::flip(image
								, mirroredImage
								, flipImageMode == FlipImage::X ? 1 : 0);
						}
						else {
							mirroredImage = image;
						}

						if (!boardInWorld->findBoard(mirroredImage
							, capture->imagePoints
							, capture->objectPoints
							, capture->worldPoints
							, this->parameters.findBoard.mode.get())) {
							throw(Exception("Failed to find board in reflection"));
						}
					}

					// Flip the image points back
					{
						switch (flipImageMode) {
						case FlipImage::X:
						{
							for (auto& imagePoint : capture->imagePoints) {
								imagePoint.x = image.cols - 1 - imagePoint.x;
							}
							break;
						}
						case FlipImage::Y:
						{
							for (auto& imagePoint : capture->imagePoints) {
								imagePoint.y = image.rows - 1 - imagePoint.y;
							}
							break;
						}
						default:
							break;
						}
					}

					// Calculate camera rays associated with these image points / world points
					{
						// unproject rays
						const auto cameraView = camera->getViewInWorldSpace();
						cameraView.castPixels(ofxCv::toOf(capture->imagePoints)
							, capture->cameraRays);

						// set the colors
						for (auto& ray : capture->cameraRays) {
							ray.color = capture->color;
						}
					}

					this->captures.add(capture);
				}
			}
		}
	}
}
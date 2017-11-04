#include "pch_Plugin_Calibration.h"

#include "CameraIntrinsics.h"

#include "ofxRulr/Nodes/Item/AbstractBoard.h"
#include "ofxRulr/Nodes/Item/Camera.h"

#include "ofxRulr/Utils/ScopedProcess.h"

#include "ofConstants.h"
#include "ofxCvGui.h"

using namespace ofxRulr::Graph;
using namespace ofxRulr::Nodes;

using namespace ofxCvGui;

using namespace ofxCv;
using namespace cv;

namespace ofxRulr {
	namespace Nodes {
		namespace Procedure {
			namespace Calibrate {
#pragma mark Capture
				//----------
				CameraIntrinsics::Capture::Capture() {
					RULR_SERIALIZE_LISTENERS;
				}

				//----------
				string CameraIntrinsics::Capture::getDisplayString() const {
					stringstream ss;
					ss << this->pointsImageSpace.size() << " points. " << endl;
					ss << std::fixed << std::showpoint << std::setprecision(2) << this->reprojectionError.get() << "px error";
					return ss.str();
				}

				//----------
				void CameraIntrinsics::Capture::drawWorldStage() {
					ofPushMatrix();
					{
						ofMultMatrix(this->extrsinsics);
						drawCorners(this->pointsObjectSpace, false);
					}
					ofPopMatrix();
				}

				//----------
				void CameraIntrinsics::Capture::drawOnImage() const {
					drawCorners(this->pointsImageSpace, false);
				}

				//----------
				void CameraIntrinsics::Capture::serialize(Json::Value & json) {
					json << this->imageWidth;
					json << this->imageHeight;
					json << this->extrsinsics;
					json << this->reprojectionError;

					json["pointsImageSpace"] << this->pointsImageSpace;
					json["pointsObjectSpace"] << this->pointsObjectSpace;
				}

				//----------
				void CameraIntrinsics::Capture::deserialize(const Json::Value & json) {
					json >> this->imageWidth;
					json >> this->imageHeight;
					json >> this->extrsinsics;
					json >> this->reprojectionError;

					json["pointsImageSpace"] >> this->pointsImageSpace;
					json["pointsObjectSpace"] >> this->pointsObjectSpace;
				}

#pragma mark CameraIntrinsics
				//----------
				CameraIntrinsics::CameraIntrinsics() {
					RULR_NODE_INIT_LISTENER;
				}

				//----------
				void CameraIntrinsics::init() {
					RULR_NODE_UPDATE_LISTENER;
					RULR_NODE_SERIALIZATION_LISTENERS;
					RULR_NODE_INSPECTOR_LISTENER;
					RULR_NODE_DRAW_WORLD_LISTENER;

					this->addInput(MAKE(Pin<Item::Camera>));
					this->addInput(MAKE(Pin<Item::AbstractBoard>));

					this->view = ofxCvGui::Panels::makeTexture(this->preview);
					this->view->onDrawImage += [this](DrawImageArguments & args) {
						auto camera = this->getInput<Item::Camera>();
						if (camera) {
							if (this->getRunFinderEnabled()) {
								ofPushMatrix();
								{
									//draw past corners
									ofPushStyle();
									{
										auto selectedCaptures = this->captures.getSelection();

										for (auto & selectedCapture : selectedCaptures) {
											ofSetColor(selectedCapture->color);
											selectedCapture->drawOnImage();
										}
									}

									//draw current corners on top
									ofxCv::drawCorners(this->currentImagePoints);

									ofPopStyle();
								}
								ofPopMatrix();
							}
						}
					};
					this->view->onDraw += [this](DrawArguments & args) {
						if (!this->getRunFinderEnabled()) {
							ofxCvGui::Utils::drawText("Select this node and connect active camera.", args.localBounds);
						}
					};
				}

				//----------
				string CameraIntrinsics::getTypeName() const {
					return "Procedure::Calibrate::CameraIntrinsics";
				}

				//----------
				ofxCvGui::PanelPtr CameraIntrinsics::getPanel() {
					return this->view;
				}

				//----------
				void CameraIntrinsics::update() {
					this->isFrameNew = false;

					if (this->isBeingInspected()) {
						auto camera = this->getInput<Item::Camera>();
						if (camera) {
							auto grabber = camera->getGrabber();
							if (grabber->isFrameNew()) {
								if (this->parameters.capture.checkAllIncomingFrames) {
									try {
										this->findBoard();

										//we need at least 4 found points for calibrateCamera to be happy to use this board find
										if (this->currentImagePoints.size() >= 4) {
											this->isFrameNew = true;
										}
										if (this->isFrameNew
											&& this->parameters.capture.tetheredShootEnabled
											&& !grabber->getDeviceSpecification().supports(ofxMachineVision::CaptureSequenceType::Continuous)
											&& grabber->getDeviceSpecification().supports(ofxMachineVision::CaptureSequenceType::OneShot)) {
											Utils::ScopedProcess scopedProcessTethered("Tethered shoot find board");
											this->addCapture(true);
											scopedProcessTethered.end();
										}
									}
									RULR_CATCH_ALL_TO_ERROR;
								}
							}
						}
					}
				}

				//----------
				void CameraIntrinsics::drawWorldStage() {
					ofPushMatrix();
					{
						//transform into camera frame of reference
						{
							auto cameraNode = this->getInput<Item::Camera>();
							if (cameraNode) {
								ofMultMatrix(cameraNode->getTransform());
							}
						}


						ofPushStyle();
						{
							auto selectedCaptures = this->captures.getSelection();
							for (auto capture : selectedCaptures) {
								ofSetColor(capture->color);
								capture->drawWorldStage();
							}
						}
						ofPopStyle();
					}
					ofPopMatrix();

				}

				//----------
				void CameraIntrinsics::serialize(Json::Value & json) {
					this->captures.serialize(json["captureSet"]);

					Utils::Serializable::serialize(json, this->error);
					Utils::Serializable::serialize(json, this->parameters);
				}

				//----------
				void CameraIntrinsics::deserialize(const Json::Value & json) {
					this->captures.deserialize(json["captureSet"]);
					
					Utils::Serializable::deserialize(json, this->error);
					Utils::Serializable::deserialize(json, this->parameters);
				}

				//----------
				bool CameraIntrinsics::getRunFinderEnabled() const {
					auto camera = this->getInput<Item::Camera>();
					if(!camera) {
						return false;
					}
					auto grabber = camera->getGrabber();
					if(!grabber->getIsDeviceOpen()) {
						return false;
					}

					else if (this->isBeingInspected()) {
						//find when we're selected
						return true;
					}
					else {
						if (camera) {
							auto grabber = camera->getGrabber();
							if (!grabber->getDeviceSpecification().supports(ofxMachineVision::CaptureSequenceType::Continuous)) {
								//find when the camera is a single shot camera
								return true;
							}
						}
					}
					
					//if nothing was good, then return false
					return false;
				}

				//----------
				void CameraIntrinsics::populateInspector(ofxCvGui::InspectArguments & inspectArguments) {
					auto inspector = inspectArguments.inspector;
					
					inspector->addIndicator("New frame found", [this]() {
						return (Widgets::Indicator::Status) !this->isFrameNew;
					});
					inspector->addIndicator("Points available", [this]() {
						return (Widgets::Indicator::Status) !this->currentImagePoints.empty();
					});
					inspector->addButton("Add capture", [this]() {
						try {
							ofxRulr::Utils::ScopedProcess scopedProcess("Finding checkerboard");
							this->addCapture(false);
							scopedProcess.end();
						}
						RULR_CATCH_ALL_TO_ERROR;
					}, ' ');

					inspector->addTitle("Captures", Widgets::Title::H2);
					this->captures.populateWidgets(inspector);

					inspector->addSpacer();

					auto calibrateButton = new Widgets::Button("Calibrate", [this]() {
						try {
							ofxRulr::Utils::ScopedProcess scopedProcess("Computing camera calibration...");
							this->calibrate();
							scopedProcess.end();
						}
						RULR_CATCH_ALL_TO_ALERT
					}, OF_KEY_RETURN);
					calibrateButton->setHeight(100.0f);
					inspector->add(calibrateButton);
					inspector->addLiveValue<float>("Reprojection error [px]", [this]() {
						return this->error;
					});

					inspector->addSpacer();

					inspector->addParameterGroup(this->parameters);
				}

				//----------
				void CameraIntrinsics::addCapture(bool triggeredFromTetheredCapture) {
					this->throwIfMissingAnyConnection();
					
					auto camera = this->getInput<Item::Camera>();
					const auto cameraSpecification = camera->getGrabber()->getDeviceSpecification();

					//if it's a DSLR, let's take a single shot and find the board
					if (cameraSpecification.supports(ofxMachineVision::CaptureSequenceType::OneShot) && !triggeredFromTetheredCapture) {
						camera->getFreshFrame();
						this->findBoard();
					}

					if (!this->isFrameNew && !this->parameters.capture.checkAllIncomingFrames) {
						//in this case let's try again to capture
						camera->getGrabber()->getFreshFrame();
					}
					
					if (this->currentImagePoints.empty()) {
						throw(ofxRulr::Exception("No corners found"));
					}

					{
						auto capture = make_shared<Capture>();
						capture->pointsImageSpace = this->currentImagePoints;
						capture->pointsObjectSpace = this->currentObjectPoints;
						capture->imageWidth = camera->getWidth();
						capture->imageHeight = camera->getHeight();
						this->captures.add(capture);
					}
				}
				//----------
				void CameraIntrinsics::findBoard() {
					this->throwIfMissingAnyConnection();

					auto camera = this->getInput<Item::Camera>();
					auto board = this->getInput<Item::AbstractBoard>();

					auto grabber = camera->getGrabber();
					auto frame = grabber->getFrame();

					//copy the frame out
					if (!frame) {
						throw(Exception("No camera frame available"));
					}
					auto & pixels = frame->getPixels();
					if (!pixels.isAllocated()) {
						throw(Exception("Camera pixels are not allocated. Perhaps we need to wait for a frame?"));
					}
					this->preview.loadData(pixels);

					this->currentImagePoints.clear();
					this->currentObjectPoints.clear();
					board->findBoard(toCv(pixels)
						, toCv(this->currentImagePoints)
						, toCv(this->currentObjectPoints)
						, this->parameters.capture.findBoardMode
						, camera->getCameraMatrix()
						, camera->getDistortionCoefficients());
				}
				
				//----------
				void CameraIntrinsics::calibrate() {
					this->throwIfMissingAConnection<Item::Camera>();

					auto allCaptures = this->captures.getAllCaptures();
					
					if (allCaptures.size() < 2) {
						throw(ofxRulr::Exception("You need to add at least 2 captures before trying to calibrate (ideally 10)"));
					}
					auto camera = this->getInput<Item::Camera>();

					vector<vector<Point2f>> imagePoints;
					vector<vector<Point3f>> objectPoints;
					for (auto & capture : allCaptures) {
						imagePoints.push_back(toCv(capture->pointsImageSpace));
						objectPoints.push_back(toCv(capture->pointsObjectSpace));
					}

					cv::Size cameraResolution(camera->getWidth(), camera->getHeight());
					Mat cameraMatrix = Mat::eye(3, 3, CV_64F);
					Mat distortionCoefficients = Mat::zeros(8, 1, CV_64F);

					vector<Mat> Rotations, Translations;
					auto flags = CV_CALIB_FIX_K6 | CV_CALIB_FIX_K5;
					this->error = cv::calibrateCamera(objectPoints, imagePoints, cameraResolution, cameraMatrix, distortionCoefficients, Rotations, Translations, flags);
					camera->setIntrinsics(cameraMatrix, distortionCoefficients);

					for (int i = 0; i < allCaptures.size(); i++) {
						auto capture = allCaptures[i];
						capture->extrsinsics = makeMatrix(Rotations[i], Translations[i]);
						
						vector<Point2f> reprojectedImageCoordinates;
						cv::projectPoints(toCv(capture->pointsObjectSpace), Rotations[i], Translations[i], cameraMatrix, distortionCoefficients, reprojectedImageCoordinates);
						float reprojectionErrorSquaredSum = 0.0f;
						for (int i = 0; i < reprojectedImageCoordinates.size(); i++) {
							reprojectionErrorSquaredSum += (toOf(reprojectedImageCoordinates[i]) - capture->pointsImageSpace[i]).lengthSquared();
						}
						capture->reprojectionError = sqrt(reprojectionErrorSquaredSum / (float)reprojectedImageCoordinates.size());
					}
				}
			}
		}
	}
}
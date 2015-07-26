#include "CameraIntrinsics.h"

#include "../../Item/Board.h"
#include "../../Item/Camera.h"

#include "ofxRulr/Utils/Utils.h"

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
				//----------
				CameraIntrinsics::CameraIntrinsics() {
					RULR_NODE_INIT_LISTENER;
				}

				//----------
				void CameraIntrinsics::init() {
					RULR_NODE_UPDATE_LISTENER;
					RULR_NODE_SERIALIZATION_LISTENERS;
					RULR_NODE_INSPECTOR_LISTENER;

					this->addInput(MAKE(Pin<Item::Board>));
					this->addInput(MAKE(Pin<Item::Camera>));

					this->view = MAKE(ofxCvGui::Panels::Base);
					this->view->onDraw += [this](DrawArguments & drawArgs) {
						auto camera = this->getInput<Item::Camera>();
						if (camera) {
							auto grabber = camera->getGrabber();
							if (this->grayscale.isAllocated()) {
								this->grayscale.draw(drawArgs.localBounds);
							}

							ofPushMatrix();
							ofScale(drawArgs.localBounds.getWidth() / camera->getWidth(), drawArgs.localBounds.getHeight() / camera->getHeight());

							//draw current corners
							ofxCv::drawCorners(this->currentCorners);

							//draw past corners
							ofPushStyle();
							ofFill();
							ofSetLineWidth(0.0f);
							int boardIndex = 0;
							ofColor boardColor(200, 100, 100);
							for (auto & board : this->accumulatedCorners) {
								boardColor.setHue(boardIndex++ * 30 % 360);
								ofSetColor(boardColor);
								for (auto & corner : board) {
									ofCircle(corner, 3.0f);
								}
							}
							ofPopStyle();

							ofPopMatrix();

							if (!this->isBeingInspected()) {
								ofEnableAlphaBlending();
								ofSetColor(255, 100);
								if (grabber->getDeviceSpecification().supports(ofxMachineVision::Feature::Feature_FreeRun)) {
									ofxCvGui::Utils::drawText("Select node to run preview...", drawArgs.localBounds);
								}
								ofDisableAlphaBlending();
							}
						}
						
					};

					this->error.set("Reprojection error", 0.0f, 0.0f, std::numeric_limits<float>::max());

					this->error = 0.0f;
				}

				//----------
				string CameraIntrinsics::getTypeName() const {
					return "Procedure::Calibrate::CameraIntrinsics";
				}

				//----------
				ofxCvGui::PanelPtr CameraIntrinsics::getView() {
					return this->view;
				}

				//----------
				void CameraIntrinsics::update() {
					if (this->isBeingInspected()) {
						auto camera = this->getInput<Item::Camera>();
						if (camera) {
							auto grabber = camera->getGrabber();
							if (grabber->getDeviceSpecification().supports(ofxMachineVision::Feature::Feature_FreeRun)) {
								try {
									this->findBoard();
								}
								RULR_CATCH_ALL_TO_ERROR
							}
						}
					}
				}

				//----------
				void CameraIntrinsics::serialize(Json::Value & json) {
					auto & jsonCorners = json["corners"];
					for (int i = 0; i<this->accumulatedCorners.size(); i++) {
						for (int j = 0; j<this->accumulatedCorners[i].size(); j++) {
							jsonCorners[i][j]["x"] = accumulatedCorners[i][j].x;
							jsonCorners[i][j]["y"] = accumulatedCorners[i][j].y;
						}
					}
					Utils::Serializable::serialize(this->error, json);
				}

				//----------
				void CameraIntrinsics::deserialize(const Json::Value & json) {
					this->accumulatedCorners.clear();

					auto & jsonBoards = json["corners"];
					for (auto & jsonBoard : jsonBoards) {
						auto board = vector<ofVec2f>();
						for (auto & jsonCorner : jsonBoard) {
							board.push_back(ofVec2f(jsonCorner["x"].asFloat(), jsonCorner["y"].asFloat()));
						}
						this->accumulatedCorners.push_back(board);
					}
					Utils::Serializable::deserialize(this->error, json);
				}

				//----------
				void CameraIntrinsics::populateInspector(ofxCvGui::ElementGroupPtr inspector) {
					inspector->add(Widgets::Indicator::make("Points found", [this]() {
						return (Widgets::Indicator::Status) !this->currentCorners.empty();
					}));
					inspector->add(Widgets::LiveValue<int>::make("Calibration set count", [this]() {
						return (int)accumulatedCorners.size();
					}));
					inspector->add(Widgets::Button::make("Add board to calibration set", [this]() {
						auto camera = this->getInput<Item::Camera>();
						if (camera) {
							//if it's a DSLR, let's take a single shot and find the board
							const auto cameraSpecification = camera->getGrabber()->getDeviceSpecification();
							if (cameraSpecification.supports(ofxMachineVision::Feature::Feature_OneShot) && !cameraSpecification.supports(ofxMachineVision::Feature::Feature_FreeRun)) {
								//by calling getFreshFrame(), then update(), we should guarantee that the frame 
								camera->getGrabber()->getFreshFrame();
								this->findBoard();
							}
						}
						if (this->currentCorners.empty()) {
							Utils::playFailSound();
						}
						else {
							Utils::playSuccessSound();
							this->accumulatedCorners.push_back(currentCorners);
						}
					}, ' '));
					inspector->add(Widgets::Button::make("Clear calibration set", [this]() {
						this->accumulatedCorners.clear();
					}));

					inspector->add(Widgets::Spacer::make());

					auto calibrateButton = Widgets::Button::make("Calibrate", [this]() {
						try {
							this->calibrate();
						}
						RULR_CATCH_ALL_TO_ALERT
					}, OF_KEY_RETURN);
					calibrateButton->setHeight(100.0f);
					inspector->add(calibrateButton);
					inspector->add(Widgets::LiveValue<float>::make("Reprojection error [px]", [this]() {
						return this->error;
					}));
				}

				//----------
				void CameraIntrinsics::findBoard() {
					this->throwIfMissingAnyConnection();

					auto camera = this->getInput<Item::Camera>();
					auto board = this->getInput<Item::Board>();

					auto grabber = camera->getGrabber();
					auto frame = grabber->getFrame();

					//copy the frame out
					frame->lockForReading();
					auto & pixels = frame->getPixels();
					if (!pixels.isAllocated()) {
						frame->unlock();
						throw(Exception("Camera pixels are not allocated. Perhaps we need to wait for a frame?"));
					}
					if (this->grayscale.getWidth() != pixels.getWidth() || this->grayscale.getHeight() != pixels.getHeight()) {
						this->grayscale.allocate(pixels.getWidth(), pixels.getHeight(), OF_IMAGE_GRAYSCALE);
					}
					if (pixels.getNumChannels() != 1) {
						cv::cvtColor(toCv(pixels), toCv(this->grayscale), CV_RGB2GRAY);
					}
					else {
						this->grayscale = pixels;
					}
					frame->unlock();

					this->grayscale.update();
					this->currentCorners.clear();

					board->findBoard(toCv(this->grayscale), toCv(this->currentCorners));
				}
				
				//----------
				void CameraIntrinsics::calibrate() {
					this->throwIfMissingAConnection<Item::Camera>();
					this->throwIfMissingAConnection<Item::Board>();

					if (this->accumulatedCorners.size() == 0) {
						throw(ofxRulr::Exception("You need to add some board captures before trying to calibrate"));
					}
					auto camera = this->getInput<Item::Camera>();
					auto board = this->getInput<Item::Board>();

					auto objectPointsSet = vector<vector<Point3f>>(this->accumulatedCorners.size(), board->getObjectPoints());
					auto cameraResolution = cv::Size(camera->getWidth(), camera->getHeight());

					Mat cameraMatrix = Mat::eye(3, 3, CV_64F);
					Mat distortionCoefficients = Mat::zeros(8, 1, CV_64F);

					vector<Mat> Rotations, Translations;
					vector<vector<Point2f>> accumulatedCornersCv = toCv(this->accumulatedCorners);
					auto flags = CV_CALIB_FIX_K6 | CV_CALIB_FIX_K5;
					this->error = cv::calibrateCamera(objectPointsSet, accumulatedCornersCv, cameraResolution, cameraMatrix, distortionCoefficients, Rotations, Translations, flags);
					camera->setIntrinsics(cameraMatrix, distortionCoefficients);
				}
			}
		}
	}
}
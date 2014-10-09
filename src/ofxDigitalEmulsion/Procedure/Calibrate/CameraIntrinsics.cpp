#include "CameraIntrinsics.h"

#include "../../Item/Board.h"
#include "../../Item/Camera.h"

#include "../../Utils/Utils.h"

#include "ofConstants.h"
#include "ofxCvGui.h"

using namespace ofxDigitalEmulsion::Graph;
using namespace ofxCvGui;

using namespace ofxCv;
using namespace cv;

namespace ofxDigitalEmulsion {
	namespace Procedure {
		namespace Calibrate {
			//----------
			CameraIntrinsics::CameraIntrinsics() {
				this->addInput(MAKE(Pin<Item::Board>));
				this->addInput(MAKE(Pin<Item::Camera>));

				this->enableFinder.set("Run board finder", false);
				this->error.set("Reprojection error", 0.0f, 0.0f, std::numeric_limits<float>::max());

				this->error = 0.0f;
			}

			//----------
			string CameraIntrinsics::getTypeName() const {
				return "CameraIntrinsics";
			}

			//----------
			ofxCvGui::PanelPtr CameraIntrinsics::getView() {
				auto view = MAKE(ofxCvGui::Panels::Base);
				view->onDraw += [this] (DrawArguments & drawArgs) {
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
						for(auto & board : this->accumulatedCorners) {
							boardColor.setHue(boardIndex++ * 30 % 360);
							ofSetColor(boardColor);
							for(auto & corner : board) {
								ofCircle(corner, 3.0f);
							}
						}
						ofPopStyle();

						ofPopMatrix();
					}
				};
				return view;
			}

			//----------
			void CameraIntrinsics::update() {
				auto camera = this->getInput<Item::Camera>();
				auto board = this->getInput<Item::Board>();

				if (this->enableFinder && camera && board) {
					try {
						auto grabber = camera->getGrabber();
						if (! grabber->getPixelsRef().isAllocated()) {
							throw(Utils::Exception("Camera pixels are not allocated. Perhaps we need to wait for a frame?"));
						}
						if (this->grayscale.getWidth() != camera->getWidth() || this->grayscale.getHeight() != camera->getHeight()) {
							this->grayscale.allocate(camera->getWidth(), camera->getHeight(), OF_IMAGE_GRAYSCALE);
						}
						if (grabber->getPixelsRef().getNumChannels() != 1) {
							cv::cvtColor(toCv(grabber->getPixelsRef()), toCv(this->grayscale), CV_RGB2GRAY);
						} else {
							this->grayscale = grabber->getPixelsRef();
						}
						this->grayscale.update();
						this->currentCorners.clear();

						board->findBoard(toCv(this->grayscale), toCv(this->currentCorners));
					}
					catch (std::exception e) {
						ofLogWarning() << e.what();
					}
				} else {
					this->currentCorners.clear();
				}
			}

			//----------
			void CameraIntrinsics::serialize(Json::Value & json) {
				auto & jsonCorners = json["corners"];
				for(int i=0; i<this->accumulatedCorners.size(); i++) {
					for(int j=0; j<this->accumulatedCorners[i].size(); j++) {
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
				for(auto & jsonBoard : jsonBoards) {
					auto board = vector<ofVec2f>();
					for(auto & jsonCorner : jsonBoard) {
						board.push_back(ofVec2f(jsonCorner["x"].asFloat(), jsonCorner["y"].asFloat()));
					}
					this->accumulatedCorners.push_back(board);
				}
				Utils::Serializable::deserialize(this->error, json);
			}

			//----------
			void CameraIntrinsics::populateInspector2(ofxCvGui::ElementGroupPtr inspector) {
				inspector->add(Widgets::Toggle::make(this->enableFinder));
				inspector->add(Widgets::Indicator::make("Points found", [this]() {
					return (Widgets::Indicator::Status) !this->currentCorners.empty();
				}));
				inspector->add(Widgets::LiveValue<int>::make("Calibration set count", [this] () {
					return (int) accumulatedCorners.size();
				}));
				inspector->add(Widgets::Button::make("Add board to calibration set", [this] () {
					if (this->currentCorners.empty()) {
						Utils::playFailSound();
					}
					else {
						Utils::playSuccessSound();
						this->accumulatedCorners.push_back(currentCorners);
					}
				}, ' '));
				inspector->add(Widgets::Button::make("Clear calibration set", [this] () {
					this->accumulatedCorners.clear();
				}));

				inspector->add(Widgets::Spacer::make());

				auto calibrateButton = Widgets::Button::make("Calibrate", [this] () {
					this->calibrate();
				}, OF_KEY_RETURN);
				calibrateButton->setHeight(100.0f);
				inspector->add(calibrateButton);
				inspector->add(Widgets::LiveValue<float>::make("Reprojection error [px]", [this] () {
					return this->error;
				}));
			}

			//----------
			void CameraIntrinsics::calibrate() {
				auto camera = this->getInput<Item::Camera>();
				auto board = this->getInput<Item::Board>();

				if (camera && board) {
					auto objectPointsSet = vector<vector<Point3f>>(this->accumulatedCorners.size(), board->getObjectPoints());
					auto cameraResolution = cv::Size(camera->getWidth(), camera->getHeight());

					Mat cameraMatrix = Mat::eye(3, 3, CV_64F);
					Mat distortionCoefficients = Mat::zeros(8, 1, CV_64F);

					vector<Mat> Rotations, Translations;
					try {
						vector<vector<Point2f>> accumulatedCornersCv = toCv(this->accumulatedCorners);
						auto flags = CV_CALIB_FIX_K6 | CV_CALIB_FIX_K5;
						this->error = cv::calibrateCamera(objectPointsSet, accumulatedCornersCv, cameraResolution, cameraMatrix, distortionCoefficients, Rotations, Translations, flags);
						camera->setIntrinsics(cameraMatrix, distortionCoefficients);
					} catch (cv::Exception e) {
						ofSystemAlertDialog(e.msg);
					}
				}
			}
		}
	}
}
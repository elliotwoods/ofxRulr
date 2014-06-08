#include "CameraIntrinsics.h"

#include "../../Item/Checkerboard.h"
#include "../../Item/Camera.h"

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
				this->inputPins.push_back(MAKE(Pin<Item::Checkerboard>));
				this->inputPins.push_back(MAKE(Pin<Item::Camera>));

				this->enableFinder.set("Run chessboard finder", false);
			
				failSound.loadSound("fail.mp3");
				successSound.loadSound("success.mp3");

				this->error = 0.0f;
			}

			//----------
			string CameraIntrinsics::getTypeName() const {
				return "CameraIntrinsics";
			}

			//----------
			PinSet CameraIntrinsics::getInputPins() const {
				return this->inputPins;
			}

			//----------
			ofxCvGui::PanelPtr CameraIntrinsics::getView() {
				auto view = MAKE(ofxCvGui::Panels::Base);
				view->onDraw += [this] (DrawArguments & drawArgs) {
					auto cameraPin = this->getInputPins().get<Pin<Item::Camera>>();
					auto camera = cameraPin->getConnection();
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
				auto checkerboard = this->getInput<Item::Checkerboard>();

				if (this->enableFinder && camera && checkerboard) {
					try {
						auto grabber = camera->getGrabber();
						if (! grabber->getPixelsRef().isAllocated()) {
							throw(std::exception());
						}
						if (this->grayscale.getWidth() != camera->getWidth() || this->grayscale.getHeight() != camera->getHeight()) {
							this->grayscale.allocate(camera->getWidth(), camera->getHeight(), OF_IMAGE_GRAYSCALE);
						}
						cv::cvtColor(toCv(grabber->getPixelsRef()), toCv(this->grayscale), CV_RGB2GRAY);
						this->grayscale.update();
						this->currentCorners.clear();
						findChessboardCornersPreTest(toCv(this->grayscale), checkerboard->getSize(), toCv(this->currentCorners));
					} catch (std::exception e) {
						ofLogWarning() << e.what();
					}
				} else {
					this->currentCorners.clear();
				}
			}

			//----------
			void CameraIntrinsics::serialize(Json::Value & json) {
				for(int i=0; i<this->accumulatedCorners.size(); i++) {
					for(int j=0; j<this->accumulatedCorners[i].size(); j++) {
						json[i][j]["x"] = accumulatedCorners[i][j].x;
						json[i][j]["y"] = accumulatedCorners[i][j].y;
					}
				}
			}

			//----------
			void CameraIntrinsics::deserialize(const Json::Value & json) {
				this->accumulatedCorners.clear();

				for(auto & jsonBoard : json) {
					auto board = vector<ofVec2f>();
					for(auto & jsonCorner : jsonBoard) {
						board.push_back(ofVec2f(jsonCorner["x"].asFloat(), jsonCorner["y"].asFloat()));
					}
					this->accumulatedCorners.push_back(board);
				}
			}

			//----------
			void CameraIntrinsics::populateInspector2(ofxCvGui::ElementGroupPtr inspector) {
				inspector->add(Widgets::Toggle::make(this->enableFinder));
				inspector->add(Widgets::LiveValue<int>::make("Calibration set count", [this] () {
					return (int) accumulatedCorners.size();
				}));
				inspector->add(Widgets::LiveValueHistory::make("Corners found in current image", [this] () {
					return (float) this->currentCorners.size();
				}, true));
				inspector->add(Widgets::Button::make("Add board to calibration set", [this] () {
					if (this->currentCorners.empty()) {
						this->failSound.play();
					} else {
						this->successSound.play();
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
				auto checkerboard = this->getInput<Item::Checkerboard>();

				if (camera && checkerboard) {
					auto objectPointsSet = vector<vector<Point3f>>(this->accumulatedCorners.size(), checkerboard->getObjectPoints());
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
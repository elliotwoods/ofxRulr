#include "CameraIntrinsics.h"

#include "../../Item/Checkerboard.h"
#include "../../Item/Camera.h"

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

				this->enableFinder.set("Find chessboards in image", false);

				this->error = 0.0f;
			}

			//----------
			string CameraIntrinsics::getTypeName() const {
				return "CameraIntrinsics";
			}

			//----------
			PinSet CameraIntrinsics::getInputPins() {
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
						this->grayscale.draw(drawArgs.localBounds);

						ofPushMatrix();
						ofScale(drawArgs.localBounds.getWidth() / grabber->getWidth(), drawArgs.localBounds.getHeight() / grabber->getHeight());
						ofxCv::drawCorners(this->currentCorners);
						ofPopMatrix();
					}

					for(auto & board : this->accumulatedCorners) {
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
						if (this->grayscale.getWidth() != grabber->getWidth() || this->grayscale.getHeight() != grabber->getHeight()) {
							this->grayscale.allocate(grabber->getWidth(), grabber->getHeight(), OF_IMAGE_GRAYSCALE);
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
			void CameraIntrinsics::deserialize(Json::Value & json) {
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
				inspector->add(Widgets::LiveValueHistory::make("Corners found", [this] () {
					return (float) this->currentCorners.size();
				}, true));
				inspector->add(Widgets::Button::make("Add image to calibration set", [this] () {
					this->accumulatedCorners.push_back(currentCorners);
				}));
				inspector->add(Widgets::Button::make("Clear calibration set", [this] () {
					this->accumulatedCorners.clear();
				}));
				inspector->add(Widgets::LiveValue<int>::make("Calibration set count", [this] () {
					return (int) accumulatedCorners.size();
				}));
				inspector->add(Widgets::Button::make("Calibrate", [this] () {
					this->calibrate();
				}));
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
					auto cameraResolution = cv::Size(camera->getGrabber()->getWidth(), camera->getGrabber()->getHeight());

					Mat CameraMatrix, DistortionMatrix;
					Mat Rotations, Translations;
					try {
						auto error = cv::calibrateCamera(objectPointsSet, this->accumulatedCorners, cameraResolution, CameraMatrix, DistortionMatrix, Rotations, Translations);
					} catch (std::exception e) {
						ofSystemAlertDialog(e.what());
					}
				}
			}
		}
	}
}
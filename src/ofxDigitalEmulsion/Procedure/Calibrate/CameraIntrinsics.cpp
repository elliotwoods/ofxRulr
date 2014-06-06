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
			void CameraIntrinsics::populateInspector(ofxCvGui::ElementGroupPtr inspector) {
				inspector->add(Widgets::Title::make(this->getTypeName(), Widgets::Title::Level::H2));
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
			void CameraIntrinsics::calibrate() {

			}
		}
	}
}
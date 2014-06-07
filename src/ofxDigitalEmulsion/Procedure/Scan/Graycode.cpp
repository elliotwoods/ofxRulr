#include "Graycode.h"

#include "ofxCvGui.h"

#include "../../Item/Camera.h"
#include "../../Item/Projector.h"

#include "ofAppGLFWWindow.h"

using namespace ofxDigitalEmulsion::Graph;
using namespace ofxCvGui;

namespace ofxDigitalEmulsion {
	namespace Procedure {
		namespace Scan {
			//---------
			Graycode::Graycode() {
				this->inputPins.push_back(MAKE(Pin<Item::Camera>));
				this->inputPins.push_back(MAKE(Pin<Item::Projector>));

				this->threshold.set("Threshold", 10.0f, 0.0f, 255.0f);
				this->delay.set("Capture delay [ms]", 200.0f, 0.0f, 2000.0f);

				this->view = MAKE(Panels::Image, this->decoder.getProjectorInCamera());

				this->previewIsOfNonLivePixels = false;
			}

			//----------
			string Graycode::getTypeName() const {
				return "Graycode";
			}

			//----------
			Graph::PinSet Graycode::getInputPins() {
				return this->inputPins;
			}
			
			//----------
			PanelPtr Graycode::getView() {
				return this->view;
			}

			//----------
			void Graycode::update() {
				//update payload if we need to
				auto projector = this->getInput<Item::Projector>();
				if (projector) {
					if (this->payload.getWidth() != projector->getWidth() || this->payload.getHeight() != projector->getHeight()) {
						payload.init(projector->getWidth(), projector->getHeight());
						encoder.init(payload);
						decoder.init(payload);
					}
				}

				this->decoder.update();
			}

			//----------
			void Graycode::serialize(Json::Value & json) {
				Utils::Serializable::serialize(this->threshold, json);
				Utils::Serializable::serialize(this->delay, json);
			}

			//----------
			void Graycode::deserialize(Json::Value & json) {
				Utils::Serializable::deserialize(this->threshold, json);
				Utils::Serializable::deserialize(this->delay, json);
				this->decoder.setThreshold(this->threshold);
			}

			//----------
			bool Graycode::isReady() {
				return (this->getInput<Item::Camera>() && this->getInput<Item::Projector>() && this->payload.isAllocated());
			}

			//----------
			void Graycode::runScan() {
				auto window = dynamic_cast<ofAppGLFWWindow*>(ofGetWindowPtr());
				if (window && this->isReady()) {
					ofSetFullscreen(true);
					ofHideCursor();

					auto screenWidth = ofGetScreenWidth();
					auto screenHeight = ofGetScreenHeight();
					ofLogNotice("ofxDigitalEmulsion::Graycode") << "Sending on screen with resolution [" << screenWidth << "x" << screenHeight << "]";

					ofPushView();
					ofViewport(0.0f, 0.0f, screenWidth, screenHeight);
					ofSetupScreenOrtho(screenWidth, screenHeight);

					auto grabber = this->getInput<Item::Camera>()->getGrabber();

					this->encoder.reset();
					this->decoder.reset();
					this->decoder.setThreshold(this->threshold);
					this->message.clear();
					
					while (this->encoder >> this->message) {
						this->message.draw(0,0);
						glfwSwapBuffers(window->getGLFWWindow());
						glFlush();

						auto startWait = ofGetElapsedTimeMillis();
						while(ofGetElapsedTimeMillis() - startWait < this->delay) {
							ofSleepMillis(1);
							grabber->update();
						}

						this->decoder << grabber->getPixelsRef();
					}

					ofPopView();

					ofShowCursor();
					ofSetFullscreen(false);

					this->switchIfLookingAtDirtyView();
				}
			}
			
			//----------
			void Graycode::populateInspector2(ElementGroupPtr inspector) {
				auto scanButton = Widgets::Button::make("SCAN", [this] () { this->runScan(); }, OF_KEY_RETURN);
				scanButton->setHeight(100.0f);
				inspector->add(scanButton);
				inspector->add(Widgets::Button::make("Clear", [this] () { 
					this->decoder.clear();
					this->preview.clear();
				}));
				inspector->add(Widgets::Button::make("Save ofxGraycode::DataSet...", [this] () { 
					this->decoder.saveDataSet();
				}));
				inspector->add(Widgets::Button::make("Load ofxGraycode::DataSet...", [this] () { 
					if (this->decoder.hasData()) {
						this->decoder.saveDataSet();
					} else {
						ofSystemAlertDialog("No data to save yet. Have you scanned?");
					}
				}));

				inspector->add(Widgets::Title::make("Decoder", Widgets::Title::Level::H2));
				inspector->add(Widgets::Slider::make(this->delay));
				auto thresholdSlider = Widgets::Slider::make(this->threshold);
				thresholdSlider->addIntValidator();
				thresholdSlider->onValueChange += [this] (ofParameter<float> &) {
					this->decoder.setThreshold(this->threshold);
					this->switchIfLookingAtDirtyView();
				};
				inspector->add(thresholdSlider);

				inspector->add(Widgets::Title::make("Payload", Widgets::Title::Level::H2));
				inspector->add(Widgets::LiveValue<float>::make("Width", [this] () { return this->payload.getWidth(); }));
				inspector->add(Widgets::LiveValue<float>::make("Height", [this] () { return this->payload.getHeight(); }));

				inspector->add(Widgets::Spacer::make());
				inspector->add(Widgets::Title::make("Views", Widgets::Title::Level::H2));
				inspector->add(Widgets::Button::make("Camera in Projector", [this] () {
					this->view->setImage(this->decoder.getCameraInProjector());
					this->previewIsOfNonLivePixels = false;
				}));
				inspector->add(Widgets::Button::make("Projector in Camera", [this] () {
					this->view->setImage(this->decoder.getProjectorInCamera());
					this->previewIsOfNonLivePixels = false;
				}));
				inspector->add(Widgets::Button::make("Median", [this] () {
					this->preview = this->decoder.getDataSet().getMedian();
					this->preview.update();
					this->view->setImage(this->preview);
					this->previewIsOfNonLivePixels = true;
				}));
				inspector->add(Widgets::Button::make("Median Inverse", [this] () {
					this->preview = this->decoder.getDataSet().getMedianInverse();
					this->preview.update();
					this->view->setImage(this->preview);
					this->previewIsOfNonLivePixels = true;
				}));
				inspector->add(Widgets::Button::make("Active", [this] () {
					this->preview = this->decoder.getDataSet().getActive();
					this->preview.update();
					this->view->setImage(this->preview);
					this->previewIsOfNonLivePixels = true;
				}));
			}

			//----------
			void Graycode::switchIfLookingAtDirtyView() {
				if (this->previewIsOfNonLivePixels) {
					this->view->setImage(this->decoder.getProjectorInCamera());
					this->previewIsOfNonLivePixels = false;
				}
			}
		}
	}
}
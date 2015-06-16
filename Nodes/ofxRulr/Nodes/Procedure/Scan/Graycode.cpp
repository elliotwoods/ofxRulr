#include "Graycode.h"

#include "../../Item/Camera.h"
#include "../../Device/VideoOutput.h"
#include "ofxRulr/Exception.h"

#include "ofxCvGui.h"

#include "ofAppGLFWWindow.h"

using namespace ofxRulr::Graph;
using namespace ofxRulr::Nodes;
using namespace ofxCvGui;

namespace ofxRulr {
	namespace Procedure {
		namespace Scan {
			//---------
			Graycode::Graycode() {
				RULR_NODE_INIT_LISTENER;
			}

			//----------
			void Graycode::init() {
				RULR_NODE_UPDATE_LISTENER;
				RULR_NODE_SERIALIZATION_LISTENERS;
				RULR_NODE_INSPECTOR_LISTENER;

				this->addInput(MAKE(Pin<Item::Camera>));
				auto videoOutputPin = MAKE(Pin<Device::VideoOutput>);
				this->addInput(videoOutputPin);

				this->threshold.set("Threshold", 10.0f, 0.0f, 255.0f);
				this->delay.set("Capture delay [ms]", 200.0f, 0.0f, 2000.0f);
				this->brightness.set("Brightness [/255]", 255.0f, 0.0f, 255.0f);
				this->enablePreviewOnVideoOutput.set("Enable preview on output", false);

				this->payload.init(1, 1);
				this->decoder.init(payload);
				this->encoder.init(payload);

				this->view = MAKE(Panels::Image, this->decoder.getProjectorInCamera());

				videoOutputPin->onNewConnection += [this](shared_ptr<Device::VideoOutput> videoOutput) {
					videoOutput->onDrawOutput.addListener([this](ofRectangle & rectangle) {
						if (this->enablePreviewOnVideoOutput) {
							this->drawPreviewOnVideoOutput(rectangle);
						}
					}, this);
				};

				videoOutputPin->onDeleteConnection += [this](shared_ptr<Device::VideoOutput> videoOutput) {
					if (videoOutput) {
						videoOutput->onDrawOutput.removeListeners(this);
					}
				};

				this->previewIsOfNonLivePixels = false;
			}
			
			//----------
			string Graycode::getTypeName() const {
				return "Procedure::Scan::Graycode";
			}

			//----------
			PanelPtr Graycode::getView() {
				return this->view;
			}

			//----------
			void Graycode::update() {
				this->decoder.update();
			}

			//----------
			void Graycode::serialize(Json::Value & json) {
				Utils::Serializable::serialize(this->threshold, json);
				Utils::Serializable::serialize(this->delay, json);
				Utils::Serializable::serialize(this->brightness, json);
				auto filename = ofFilePath::removeExt(this->getDefaultFilename()) + ".sl";
				this->decoder.saveDataSet(filename);

				Utils::Serializable::serialize(this->enablePreviewOnVideoOutput, json);
			}

			//----------
			void Graycode::deserialize(const Json::Value & json) {
				auto filename = ofFilePath::removeExt(this->getDefaultFilename()) + ".sl";
				this->decoder.loadDataSet(filename, false);
				Utils::Serializable::deserialize(this->threshold, json);
				this->decoder.setThreshold(this->threshold);
				Utils::Serializable::deserialize(this->delay, json);
				Utils::Serializable::deserialize(this->brightness, json);

				Utils::Serializable::deserialize(this->enablePreviewOnVideoOutput, json);
			}

			//----------
			bool Graycode::isReady() {
				return (this->getInput<Item::Camera>() && this->getInput<Device::VideoOutput>() && this->payload.isAllocated());
			}

			//----------
			void Graycode::runScan() {
				//safety checks
				this->throwIfMissingAnyConnection();

				//get variables
				auto camera = this->getInput<Item::Camera>();
				auto videoOutput = this->getInput<Device::VideoOutput>();
				auto videoOutputSize = videoOutput->getSize();
				auto grabber = camera->getGrabber();
				
				//check that the window is open
				if (!videoOutput->isWindowOpen()) {
					throw(Exception("Cannot run Graycode scan whilst the VideoOutput's window isn't open"));
				}

				//initialise payload
				this->payload.init(videoOutputSize.getWidth(), videoOutputSize.getHeight());
				this->encoder.init(payload);
				this->decoder.init(payload);

				//initialise scan
				this->encoder.reset();
				this->decoder.reset();
				this->decoder.setThreshold(this->threshold);
				this->message.clear();
				
				ofHideCursor();

				while (this->encoder >> this->message) {
					videoOutput->clearFbo(false);
					videoOutput->begin();
					//
					ofPushStyle();
					auto brightness = this->brightness;
					ofSetColor(brightness);
					this->message.draw(0, 0);
					ofPopStyle();
					//
					videoOutput->end();
					videoOutput->presentFbo();

					stringstream message;
					message << this->getName() << " scanning " << this->encoder.getFrame() << "/" << this->encoder.getFrameCount();
					ofxCvGui::Utils::drawProcessingNotice(message.str());

					auto startWait = ofGetElapsedTimeMillis();
					while(ofGetElapsedTimeMillis() - startWait < this->delay) {
						ofSleepMillis(1);
						grabber->update();
					}

					auto frame = grabber->getFreshFrame();
					frame->lockForReading();
					this->decoder << frame->getPixelsRef();
					frame->unlock();
				}

				ofShowCursor();

				this->switchIfLookingAtDirtyView();
			}
			
			//----------
			ofxGraycode::Decoder & Graycode::getDecoder() {
				return this->decoder;
			}

			//----------
			const ofxGraycode::DataSet & Graycode::getDataSet() const {
				if (!this->decoder.hasData()) {
					throw(Exception("Can't get DataSet from Graycode node, no data available"));
				}
				return this->decoder.getDataSet();
			}

			//----------
			void Graycode::drawPreviewOnVideoOutput(const ofRectangle & rectangle) {
				auto preview = this->view->getImage();
				if (preview) {
					preview->draw(rectangle);
				}
			}

			//----------
			void Graycode::populateInspector(ElementGroupPtr inspector) {
				auto scanButton = Widgets::Button::make("SCAN", [this] () {
					try {
						this->runScan();
					} 
					RULR_CATCH_ALL_TO_ALERT
				}, OF_KEY_RETURN);
				scanButton->setHeight(100.0f);
				inspector->add(scanButton);
				inspector->add(Widgets::Button::make("Clear", [this] () { 
					this->decoder.clear();
					this->preview.clear();
				}));
				inspector->add(Widgets::Button::make("Save ofxGraycode::DataSet...", [this] () { 
					if (this->decoder.hasData()) {
						this->decoder.saveDataSet();
						this->decoder.savePreviews();
					} else {
						ofSystemAlertDialog("No data to save yet. Have you scanned?");
					}
				}));
				inspector->add(Widgets::Button::make("Load ofxGraycode::DataSet...", [this] () { 
					this->decoder.loadDataSet();
				}));

				inspector->add(Widgets::Title::make("Decoder", Widgets::Title::Level::H2));
				inspector->add(Widgets::LiveValue<string>::make("Has data", [this] () {
					return this->decoder.hasData() ? "True" : "False";
				}));
				inspector->add(Widgets::Slider::make(this->delay));
				auto thresholdSlider = Widgets::Slider::make(this->threshold);
				thresholdSlider->addIntValidator();
				thresholdSlider->onValueChange += [this] (ofParameter<float> &) {
					this->decoder.setThreshold(this->threshold);
					this->switchIfLookingAtDirtyView();
				};
				inspector->add(thresholdSlider);
				auto brightnessSlider = Widgets::Slider::make(this->brightness);
				brightnessSlider->addIntValidator();
				inspector->add(brightnessSlider);

				inspector->add(Widgets::Title::make("Payload", Widgets::Title::Level::H2));
				inspector->add(Widgets::LiveValue<unsigned int>::make("Width", [this] () { return this->payload.getWidth(); }));
				inspector->add(Widgets::LiveValue<unsigned int>::make("Height", [this]() { return this->payload.getHeight(); }));

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

				inspector->add(Widgets::Spacer::make());

				inspector->add(Widgets::Toggle::make(this->enablePreviewOnVideoOutput));
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
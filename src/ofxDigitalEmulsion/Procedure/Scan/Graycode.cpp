#include "Graycode.h"

#include "../../Item/Camera.h"
#include "../../Device/VideoOutput.h"
#include "../../Utils/Exception.h"

#include "ofxCvGui.h"

#include "ofAppGLFWWindow.h"

using namespace ofxDigitalEmulsion::Graph;
using namespace ofxCvGui;

namespace ofxDigitalEmulsion {
	namespace Procedure {
		namespace Scan {
			//---------
			Graycode::Graycode() {

			}

			//----------
			void Graycode::init() {
				this->addInput(MAKE(Pin<Item::Camera>));
				this->addInput(MAKE(Pin<Device::VideoOutput>));

				this->threshold.set("Threshold", 10.0f, 0.0f, 255.0f);
				this->delay.set("Capture delay [ms]", 200.0f, 0.0f, 2000.0f);

				this->view = MAKE(Panels::Image, this->decoder.getProjectorInCamera());

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
				//update payload if we need to
				auto videoOut = this->getInput<Device::VideoOutput>();
				if (videoOut) {
					const auto outputSize = videoOut->getSize();
					if (this->payload.getWidth() != outputSize.getWidth() || this->payload.getHeight() != outputSize.getHeight()) {
						payload.init(outputSize.getWidth(), outputSize.getHeight());
						encoder.init(payload);
						decoder.init(payload);
						this->load(this->getDefaultFilename());
					}
				}

				this->decoder.update();
			}

			//----------
			void Graycode::serialize(Json::Value & json) {
				Utils::Serializable::serialize(this->threshold, json);
				Utils::Serializable::serialize(this->delay, json);
				auto filename = ofFilePath::removeExt(this->getDefaultFilename()) + ".sl";
				this->decoder.saveDataSet(filename);
			}

			//----------
			void Graycode::deserialize(const Json::Value & json) {
				auto filename = ofFilePath::removeExt(this->getDefaultFilename()) + ".sl";
				this->decoder.loadDataSet(filename);
				Utils::Serializable::deserialize(this->threshold, json);
				Utils::Serializable::deserialize(this->delay, json);
				this->decoder.setThreshold(this->threshold);
			}

			//----------
			bool Graycode::isReady() {
				return (this->getInput<Item::Camera>() && this->getInput<Device::VideoOutput>() && this->payload.isAllocated());
			}

			//----------
			void Graycode::runScan() {
				//safety checks
				this->throwIfMissingAnyConnection();
				if (!this->payload.isAllocated()) {
					throw(Utils::Exception("Payload is not allocated"));
				}

				//get variables
				auto window = glfwGetCurrentContext();
				auto camera = this->getInput<Item::Camera>();
				auto videoOutput = this->getInput<Device::VideoOutput>();
				auto videoOutputSize = videoOutput->getSize();
				auto grabber = camera->getGrabber();

				//initialise scan
				this->payload.init(videoOutputSize.getWidth(), videoOutputSize.getHeight());
				this->encoder.reset();
				this->decoder.reset();
				this->decoder.setThreshold(this->threshold);
				this->message.clear();
				
				ofHideCursor();

				while (this->encoder >> this->message) {
					videoOutput->clearFbo(false);
					videoOutput->begin();
					this->message.draw(0, 0);
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
					this->decoder << frame->getPixelsRef();
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
					throw(Utils::Exception("Can't get DataSet from Graycode node, no data available"));
				}
				return this->decoder.getDataSet();
			}
			
			//----------
			void Graycode::populateInspector2(ElementGroupPtr inspector) {
				auto scanButton = Widgets::Button::make("SCAN", [this] () {
					try {
						this->runScan();
					} 
					OFXDIGITALEMULSION_CATCH_ALL_TO_ALERT
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
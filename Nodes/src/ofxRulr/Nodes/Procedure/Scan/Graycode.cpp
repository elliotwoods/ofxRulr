#include "Graycode.h"

#include "ofxRulr/Nodes/Item/Camera.h"
#include "ofxRulr/Nodes/System/VideoOutput.h"
#include "ofxRulr/Exception.h"

#include "ofxCvGui.h"

#include "ofAppGLFWWindow.h"

using namespace ofxRulr::Graph;
using namespace ofxRulr::Nodes;
using namespace ofxCvGui;

namespace ofxRulr {
	namespace Nodes {
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
					auto videoOutputPin = MAKE(Pin<System::VideoOutput>);
					this->addInput(videoOutputPin);

					this->threshold.set("Threshold", 10.0f, 0.0f, 255.0f);
					this->delay.set("Capture delay [ms]", 200.0f, 0.0f, 2000.0f);
					this->brightness.set("Brightness [/255]", 255.0f, 0.0f, 255.0f);
					this->enablePreviewOnVideoOutput.set("Enable preview on output", false);

					this->payload.init(1, 1);
					this->decoder.init(payload);
					this->encoder.init(payload);

					this->view = MAKE(Panels::Draws, this->preview);

					videoOutputPin->onNewConnection += [this](shared_ptr<System::VideoOutput> videoOutput) {
						videoOutput->onDrawOutput.addListener([this](ofRectangle & rectangle) {
							if (this->enablePreviewOnVideoOutput) {
								this->drawPreviewOnVideoOutput(rectangle);
							}
						}, this);
					};

					videoOutputPin->onDeleteConnection += [this](shared_ptr<System::VideoOutput> videoOutput) {
						if (videoOutput) {
							videoOutput->onDrawOutput.removeListeners(this);
						}
					};
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
					
					if(this->previewDirty) {
						this->updatePreview();
					}
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
					
					this->previewDirty = true;
				}

				//----------
				bool Graycode::isReady() {
					return (this->getInput<Item::Camera>() && this->getInput<System::VideoOutput>() && this->payload.isAllocated());
				}

				//----------
				void Graycode::runScan() {
					//safety checks
					this->throwIfMissingAnyConnection();

					//get variables
					auto camera = this->getInput<Item::Camera>();
					auto videoOutput = this->getInput<System::VideoOutput>();
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
#ifdef TARGET_OSX
/*
 something strange on OSX
 We found that to flush the video output we need to call:
 
 videoOutput->presentFbo()
 some waiting
 grabber->update();
 some waiting
 videoOutput->presentFbo();
 
 so we just do this part twice
 */
						for(int i=0; i<2; i++) {
#endif
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
						while (ofGetElapsedTimeMillis() - startWait < this->delay) {
							ofSleepMillis(1);
							grabber->update();
						}

#ifdef TARGET_OSX
						}
#endif
						auto frame = grabber->getFreshFrame();
						this->decoder << frame->getPixels();
					}

					ofShowCursor();

					this->previewDirty = true;
				}
				
				//----------
				void Graycode::clear() {
					this->decoder.clear();
					this->previewDirty = true;
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
					this->preview.draw(rectangle);
				}

				//----------
				void Graycode::populateInspector(InspectArguments & inspectArguments) {
					auto inspector = inspectArguments.inspector;
					
					auto scanButton = Widgets::Button::make("SCAN", [this]() {
						try {
							this->runScan();
						}
						RULR_CATCH_ALL_TO_ALERT
					}, OF_KEY_RETURN);
					scanButton->setHeight(100.0f);
					inspector->add(scanButton);
					inspector->add(Widgets::Button::make("Clear", [this]() {
						this->clear();
					}));
					inspector->add(Widgets::Button::make("Save ofxGraycode::DataSet...", [this]() {
						if (this->decoder.hasData()) {
							this->decoder.saveDataSet();
							this->decoder.savePreviews();
						}
						else {
							ofSystemAlertDialog("No data to save yet. Have you scanned?");
						}
					}));
					inspector->add(Widgets::Button::make("Load ofxGraycode::DataSet...", [this]() {
						this->decoder.loadDataSet();
					}));

					inspector->add(Widgets::Title::make("Decoder", Widgets::Title::Level::H2));
					inspector->add(Widgets::LiveValue<string>::make("Has data", [this]() {
						return this->decoder.hasData() ? "True" : "False";
					}));
					inspector->add(Widgets::Slider::make(this->delay));
					auto thresholdSlider = Widgets::Slider::make(this->threshold);
					thresholdSlider->addIntValidator();
					thresholdSlider->onValueChange += [this](ofParameter<float> &) {
						this->decoder.setThreshold(this->threshold);
						this->previewDirty = true;
					};
					inspector->add(thresholdSlider);
					auto brightnessSlider = Widgets::Slider::make(this->brightness);
					brightnessSlider->addIntValidator();
					inspector->add(brightnessSlider);

					inspector->add(Widgets::Title::make("Payload", Widgets::Title::Level::H2));
					inspector->add(Widgets::LiveValue<unsigned int>::make("Width", [this]() { return this->payload.getWidth(); }));
					inspector->add(Widgets::LiveValue<unsigned int>::make("Height", [this]() { return this->payload.getHeight(); }));

					inspector->add(Widgets::Spacer::make());
					auto previewModeSelector = Widgets::MultipleChoice::make("Preview mode");
					{
						previewModeSelector->addOption("CinP");
						previewModeSelector->addOption("PinC");
						previewModeSelector->addOption("M");
						previewModeSelector->addOption("MI");
						previewModeSelector->addOption("A");
						previewModeSelector->entangle(this->previewMode);
						previewModeSelector->onValueChange += [this](const int) {
							this->previewDirty = true;
						};
						inspector->add(previewModeSelector);
						inspector->add(Widgets::Toggle::make(this->enablePreviewOnVideoOutput));
					}
					inspector->add(Widgets::LiveValue<string>::make("Mode name",[this](){
						return this->getPreviewModeString();
					}));
				}
				
				//----------
				void Graycode::updatePreview() {
					this->preview.clear();
					
					if(this->decoder.hasData()) {
						auto previewMode = static_cast<PreviewMode>(this->previewMode.get());
						switch (previewMode) {
							case PreviewMode::CameraInProjector:
							{
								this->preview.loadData(this->decoder.getCameraInProjector().getPixels());
								break;
							}
							case PreviewMode::ProjectorInCamera:
							{
								this->preview.loadData(this->decoder.getProjectorInCamera().getPixels());
								break;
							}
							case PreviewMode::Median:
							{
								this->preview.loadData(this->decoder.getDataSet().getMedian());
								break;
							}
							case PreviewMode::MedianInverse:
							{
								this->preview.loadData(this->decoder.getDataSet().getMedianInverse());
								break;
							}
							case PreviewMode::Active:
							{
								this->preview.loadData(this->decoder.getDataSet().getActive());
								break;
							}
							default:
								break;
						}
					}
					
					this->previewDirty = false;
				}
				
				//----------
#define CASEMODESTRING(X) case X: return #X
				string Graycode::getPreviewModeString() const {
					auto previewMode = static_cast<PreviewMode>(this->previewMode.get());
					switch (previewMode) {
						CASEMODESTRING(CameraInProjector);
						CASEMODESTRING(ProjectorInCamera);
						CASEMODESTRING(Median);
						CASEMODESTRING(MedianInverse);
						CASEMODESTRING(Active);
						deafult:
							return "";
					}
				}
			}
		}
	}
}
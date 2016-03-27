#include "pch_RulrNodes.h"
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
					this->videoOutputMode.set("Output mode", 1, 0, 2);
					this->previewMode.set("Preview mode", 0, 0, 4);

					this->payload.init(1, 1);
					this->decoder.init(payload);
					this->encoder.init(payload);

					this->view = MAKE(Panels::Draws, this->preview);

					videoOutputPin->onNewConnection += [this](shared_ptr<System::VideoOutput> videoOutput) {
						videoOutput->onDrawOutput.addListener([this](ofRectangle & rectangle) {
							auto videoOutputMode = static_cast<VideoOutputMode>(this->videoOutputMode.get());
							switch (videoOutputMode) {
								case TestPattern:
								{
									this->testPattern.draw(rectangle);
									break;
								}
								case Data:
									this->preview.draw(rectangle);
									break;
								default:
									break;
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
				PanelPtr Graycode::getPanel() {
					return this->view;
				}

				//----------
				void Graycode::update() {
					this->decoder.update();
					
					if(this->previewDirty) {
						this->updatePreview();
					}
					
					if(this->testPattern.getWidth() != this->payload.getWidth() || this->testPattern.getHeight() != this->payload.getHeight() || this->testPatternBrightness != this->brightness) {
						this->updateTestPattern();
					}
				}

				//----------
				void Graycode::serialize(Json::Value & json) {
					Utils::Serializable::serialize(this->threshold, json);
					Utils::Serializable::serialize(this->delay, json);
					Utils::Serializable::serialize(this->brightness, json);
					auto filename = ofFilePath::removeExt(this->getDefaultFilename()) + ".sl";
					this->decoder.saveDataSet(filename);

					Utils::Serializable::serialize(this->videoOutputMode, json);
					Utils::Serializable::serialize(this->previewMode, json);
				}

				//----------
				void Graycode::deserialize(const Json::Value & json) {
					auto filename = ofFilePath::removeExt(this->getDefaultFilename()) + ".sl";
					this->decoder.loadDataSet(filename, false);
					Utils::Serializable::deserialize(this->threshold, json);
					this->decoder.setThreshold(this->threshold);
					Utils::Serializable::deserialize(this->delay, json);
					Utils::Serializable::deserialize(this->brightness, json);
					
					Utils::Serializable::deserialize(this->videoOutputMode, json);
					Utils::Serializable::deserialize(this->previewMode, json);
					
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

					try {

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
							for (int i = 0; i < 2; i++) {
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
					}
					catch (...) {
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
					
					inspector->add(new Widgets::Title("Scan", Widgets::Title::Level::H2));
					{
						auto scanButton = new Widgets::Button("SCAN", [this]() {
							try {
								this->runScan();
							}
							RULR_CATCH_ALL_TO_ALERT
						}, OF_KEY_RETURN);
						scanButton->setHeight(100.0f);
						inspector->add(scanButton);

						inspector->add(new Widgets::Button("Clear", [this]() {
							this->clear();
						}));
						inspector->add(new Widgets::Button("Save ofxGraycode::DataSet...", [this]() {
							if (this->decoder.hasData()) {
								this->decoder.saveDataSet();
								this->decoder.savePreviews();
							}
							else {
								ofSystemAlertDialog("No data to save yet. Have you scanned?");
							}
						}));
						inspector->add(new Widgets::Button("Load ofxGraycode::DataSet...", [this]() {
							this->decoder.loadDataSet();
						}));
						
						inspector->add(new Widgets::Title("Decoder", Widgets::Title::Level::H2));
						inspector->add(new Widgets::LiveValue<string>("Has data", [this]() {
							return this->decoder.hasData() ? "True" : "False";
						}));
						inspector->add(new Widgets::Slider(this->delay));

						auto thresholdSlider = new Widgets::Slider(this->threshold);
						thresholdSlider->addIntValidator();
						thresholdSlider->onValueChange += [this](ofParameter<float> &) {
							this->decoder.setThreshold(this->threshold);
							this->previewDirty = true;
						};
						inspector->add(thresholdSlider);

						auto brightnessSlider = new Widgets::Slider(this->brightness);
						brightnessSlider->addIntValidator();
						inspector->add(brightnessSlider);
					}

					inspector->add(new Widgets::Title("Payload", Widgets::Title::Level::H2));
					{
						inspector->add(new Widgets::LiveValue<unsigned int>("Width", [this]() { return this->payload.getWidth(); }));
						inspector->add(new Widgets::LiveValue<unsigned int>("Height", [this]() { return this->payload.getHeight(); }));
					}

					inspector->add(new Widgets::Title("Scan camera", Widgets::Title::Level::H2));
					{
						inspector->add(new Widgets::LiveValue<unsigned int>("Width", [this]() { return this->getDataSet().getWidth(); }));
						inspector->add(new Widgets::LiveValue<unsigned int>("Height", [this]() { return this->getDataSet().getHeight(); }));
					}

					inspector->add(new Widgets::Spacer());
					
					inspector->add(new Widgets::Title("Preview", Widgets::Title::Level::H2));
					{
						auto videoOutputModeSelector = inspector->add(new Widgets::MultipleChoice("Video output mode"));
						{
							videoOutputModeSelector->addOption("None");
							videoOutputModeSelector->addOption("Test Patt..");
							videoOutputModeSelector->addOption("Data");
							videoOutputModeSelector->entangle(this->videoOutputMode);
						}
						auto previewModeSelector = inspector->add(new Widgets::MultipleChoice("Preview mode"));
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
						}
						inspector->add(new Widgets::LiveValue<string>("Mode name",[this](){
							return this->getPreviewModeString();
						}));
					}
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
				void Graycode::updateTestPattern() {
					this->testPattern.clear();
					if(this->payload.isAllocated()) {
						ofPixels testPatternPixels;
						
						auto width = this->payload.getWidth();
						auto height = this->payload.getHeight();
						auto value = static_cast<unsigned char>(this->brightness.get());
						
						testPatternPixels.allocate(width, height, 1);
						for(int j=0; j<height; j++) {
							auto pixel = &testPatternPixels[j * width];
							for(int i=0; i<width; i++) {
								*pixel++ = i % 2 == j % 2 ? value : 0; //checkerboard per pixel
							}
						}
						this->testPattern.loadData(testPatternPixels);
						this->testPatternBrightness = value;
					}
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
						default:
							return "";
					}
				}
			}
		}
	}
}
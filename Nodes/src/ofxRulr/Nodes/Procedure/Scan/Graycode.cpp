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
					this->parameters.preview.previewMode.addListener(this, &Graycode::callbackChangePreviewMode);
				}

				//----------
				void Graycode::init() {
					RULR_NODE_UPDATE_LISTENER;
					RULR_NODE_SERIALIZATION_LISTENERS;
					RULR_NODE_INSPECTOR_LISTENER;

					this->addInput(MAKE(Pin<Item::Camera>));
					auto videoOutputPin = MAKE(Pin<System::VideoOutput>);
					this->addInput(videoOutputPin);

					this->videoOutputListener = make_unique<Utils::VideoOutputListener>(videoOutputPin
						, [this](const ofRectangle & bounds) {
						auto videoOutputMode = static_cast<VideoOutputMode>(this->parameters.preview.videoOutputMode.get());
						switch (videoOutputMode) {
						case VideoOutputMode::TestPattern:
						{
							if (this->testPattern.isAllocated()) {
								this->testPattern.draw(bounds);
							}
							break;
						}
						case VideoOutputMode::Data:
						{
							if (this->preview.isAllocated()) {
								this->preview.draw(bounds);
							}
							break;
						}
						default:
							break;
						}
					});

					videoOutputPin->onNewConnection += [this](shared_ptr<System::VideoOutput> videoOutput) {
						//check if this invalidates an existing suite
						if (this->suite) {
							if (this->suite->payload.getWidth() != videoOutput->getWidth()
								|| this->suite->payload.getHeight() != videoOutput->getHeight()) {
								this->invalidateSuite();
							}
						}
					};

					videoOutputPin->onDeleteConnection += [this](shared_ptr<System::VideoOutput> videoOutput) {
						
					};

					this->view = ofxCvGui::Panels::makeTexture(this->preview);
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
					if(this->previewDirty) {
						this->updatePreview();
					}
					
					//check if suite has been invalidated
					if (!this->suite) {
						//check if we're eligible to make a suite
						auto videoOutput = this->getInput<System::VideoOutput>();
						if(videoOutput->isWindowOpen()) {
							//build the suite
							auto suite = make_unique<Suite>();
							suite->payload.init(videoOutput->getWidth(), videoOutput->getHeight());
							suite->encoder.init(suite->payload);
							suite->decoder.init(suite->payload);
							this->suite = move(suite);
						}
					}

					if (this->suite) {
						if (this->testPattern.getWidth() != this->suite->payload.getWidth()
							|| this->testPattern.getHeight() != this->suite->payload.getHeight()
							|| this->testPatternBrightness != this->parameters.scan.brightness) {
							this->updateTestPattern();
						}
					}
				}

				//----------
				void Graycode::serialize(Json::Value & json) {
					Utils::Serializable::serialize(json, this->parameters);
					
					if (this->suite) {
						json["hasData"] = true;

						auto & jsonPayload = json["payload"];
						jsonPayload["width"] = (int) this->suite->payload.getWidth();
						jsonPayload["height"] = (int) this->suite->payload.getHeight();

						auto filename = ofFilePath::removeExt(this->getDefaultFilename()) + ".sl";
						this->suite->decoder.saveDataSet(filename);
						json["filename"] = filename;
					}
					else {
						json["hasData"] = false;
					}
				}

				//----------
				void Graycode::deserialize(const Json::Value & json) {
					Utils::Serializable::deserialize(json, this->parameters);

					//load dataset
					{
						auto hasData = json["hasData"].asBool();
						if (hasData) {
							auto & jsonPayload = json["payload"];
							auto payloadWidth = jsonPayload["width"].asInt();
							auto payloadHeight = jsonPayload["height"].asInt();

							auto suite = make_unique<Suite>();
							suite->payload.init(payloadWidth, payloadHeight);
							suite->decoder.init(suite->payload);
							suite->encoder.init(suite->payload);

							auto filename =  json["filename"].asString();
							suite->decoder.loadDataSet(filename);

							this->suite = move(suite);
						}
					}

					//deal with parameters
					if (this->suite) {
						this->suite->decoder.setThreshold(this->parameters.processing.threshold);
					}
					
					this->previewDirty = true;
				}

				//----------
				void Graycode::throwIfNotReadyForScan() const {
					this->throwIfMissingAnyConnection();

					if (!this->getInput<System::VideoOutput>()->isWindowOpen()) {
						throw(ofxRulr::Exception("Cannot run Graycode scan whilst VideoOutput window is not open"));
					}
				}

				//----------
				void Graycode::runScan() {
					//safety checks
					this->throwIfNotReadyForScan();

					//get variables
					auto camera = this->getInput<Item::Camera>();
					auto videoOutput = this->getInput<System::VideoOutput>();
					auto videoOutputSize = videoOutput->getSize();
					auto grabber = camera->getGrabber();

					//rebuild suite
					{
						auto suite = make_unique<Suite>();
						suite->payload.init(videoOutputSize.width, videoOutputSize.height);
						suite->encoder.init(suite->payload);
						suite->decoder.init(suite->payload);
						this->suite = move(suite);
					}

					this->suite->decoder.setThreshold(this->parameters.processing.threshold);

					//clear the output
					this->message.clear();

					ofHideCursor();

					try {
						Utils::ScopedProcess scopedProcess("Scanning graycode", true, this->suite->payload.getFrameCount());

						while (this->suite->encoder >> this->message) {
							Utils::ScopedProcess frameScopedProcess("Scanning frame", false);
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
								for (int i = 0; i < this->parameters.scan.flushOutputFrames + 1; i++) {
									videoOutput->clearFbo(false);
									videoOutput->begin();
									{
										ofPushStyle();
										{
											auto brightness = this->parameters.scan.brightness;
											ofSetColor(brightness);
											this->message.draw(0, 0);
										}
										ofPopStyle();
									}
									videoOutput->end();
									videoOutput->presentFbo();
								}

								auto startWait = ofGetElapsedTimeMillis();
								while (ofGetElapsedTimeMillis() - startWait < this->parameters.scan.captureDelay) {
									ofSleepMillis(1);
									grabber->update();
								}

								for (int i = 0; i < this->parameters.scan.flushInputFrames; i++) {
									grabber->getFreshFrame();
								}
#ifdef TARGET_OSX
							}
#endif
							auto frame = grabber->getFreshFrame();
							if (!frame) {
								throw(ofxRulr::Exception("Couldn't get fresh frame from camera"));
							}
							this->suite->decoder << frame->getPixels();
						}
						scopedProcess.end();
					}
					RULR_CATCH_ALL_TO_ALERT
					catch (...) {
					}
					ofShowCursor();

					this->previewDirty = true;
				}
				
				//----------
				void Graycode::clear() {
					this->invalidateSuite();
				}

				//----------
				bool Graycode::hasData() const {
					if (this->suite) {
						return this->suite->decoder.hasData();
					}
					else {
						return false;
					}
				}

				//----------
				bool Graycode::hasScanSuite() const {
					if (this->suite) {
						return true;
					}
					else {
						return false;
					}
				}

				//----------
				ofxGraycode::Decoder & Graycode::getDecoder() const {
					if (this->suite) {
						return this->suite->decoder;
					}
					else {
						throw(ofxRulr::Exception("Decoder has not been allocated."));
					}
				}

				//----------
				const ofxGraycode::DataSet & Graycode::getDataSet() const {
					//will throw if needs be
					return this->getDecoder().getDataSet();
				}


				//----------
				void Graycode::setDataSet(const ofxGraycode::DataSet & dataSet) {
					//will throw if needs be
					this->getDecoder().setDataSet(dataSet);
					this->previewDirty = true;
				}

				//----------
				void Graycode::invalidateSuite() {
					this->suite.reset();
				}

				//----------
				void Graycode::drawPreviewOnVideoOutput(const ofRectangle & rectangle) {
					this->preview.draw(rectangle);
				}

				//----------
				void Graycode::populateInspector(InspectArguments & inspectArguments) {
					auto inspector = inspectArguments.inspector;
					
					inspector->addParameterGroup(this->parameters);

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
							if (this->suite) {
								this->suite->decoder.saveDataSet();
								this->suite->decoder.savePreviews();
							}
							else {
								ofSystemAlertDialog("No data to save yet. Have you scanned?");
							}
						}));
						inspector->add(new Widgets::Button("Load ofxGraycode::DataSet...", [this]() {
							try {
								this->getDecoder().loadDataSet();
							}
							RULR_CATCH_ALL_TO_ALERT;
						}));
						
						inspector->add(new Widgets::Title("Decoder", Widgets::Title::Level::H2));
						inspector->add(new Widgets::LiveValue<string>("Has data", [this]() {
							return this->hasData() ? "True" : "False";
						}));
					}

					inspector->add(new Widgets::Title("Payload", Widgets::Title::Level::H2));
					{
						inspector->add(new Widgets::LiveValue<unsigned int>("Width", [this]() {
							if (this->suite) {
								return this->suite->payload.getWidth();
							}
							else {
								return (uint32_t)0;
							}
						}));
						inspector->add(new Widgets::LiveValue<unsigned int>("Height", [this]() {
							if (this->suite) {
								return this->suite->payload.getHeight();
							}
							else {
								return (uint32_t)0;
							}
						}));
					}

					inspector->add(new Widgets::Title("Scan camera", Widgets::Title::Level::H2));
					{
						inspector->add(new Widgets::LiveValue<unsigned int>("Width", [this]() {
							if (this->suite) {
								return (uint32_t) this->suite->decoder.getWidth();
							}
							else {
								return (uint32_t) 0;
							}
						}));
						inspector->add(new Widgets::LiveValue<unsigned int>("Height", [this]() {
							if (this->suite) {
								return (uint32_t) this->suite->decoder.getHeight();
							}
							else {
								return (uint32_t) 0;
							}
						}));
					}
				}
				
				//----------
				void Graycode::updatePreview() {
					this->preview.clear();
					
					try {
						auto previewMode = static_cast<PreviewMode>(this->parameters.preview.previewMode.get());
						switch (previewMode) {
						case PreviewMode::CameraInProjector:
						{
							this->preview.loadData(this->getDecoder().getCameraInProjector().getPixels());
							break;
						}
						case PreviewMode::ProjectorInCamera:
						{
							this->preview.loadData(this->getDecoder().getProjectorInCamera().getPixels());
							break;
						}
						case PreviewMode::Median:
						{
							this->preview.loadData(this->getDecoder().getDataSet().getMedian());
							break;
						}
						case PreviewMode::MedianInverse:
						{
							this->preview.loadData(this->getDecoder().getDataSet().getMedianInverse());
							break;
						}
						case PreviewMode::Active:
						{
							this->preview.loadData(this->getDecoder().getDataSet().getActive());
							break;
						}
						default:
							break;
						}
					}
					catch (...) {
					}

					this->previewDirty = false;
				}

				//----------
				void Graycode::updateTestPattern() {
					this->testPattern.clear();
					if(this->suite) {
						ofPixels testPatternPixels;
						
						auto width = this->suite->payload.getWidth();
						auto height = this->suite->payload.getHeight();
						auto value = static_cast<unsigned char>(this->parameters.scan.brightness.get());
						
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
				void Graycode::callbackChangePreviewMode(PreviewMode & previewMode) {
					this->previewDirty = true;
				}
			}
		}
	}
}
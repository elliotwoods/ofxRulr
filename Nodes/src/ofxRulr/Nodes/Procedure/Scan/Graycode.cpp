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

					this->parameters.processing.threshold.addListener(this, &Graycode::callbackChangeThreshold);
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

					this->rebuild();

					this->view = MAKE(Panels::Texture, this->preview);

					videoOutputPin->onNewConnection += [this](shared_ptr<System::VideoOutput> videoOutput) {
						this->rebuild();

						videoOutput->onDrawOutput.addListener([this](ofRectangle & rectangle) {
							auto videoOutputMode = static_cast<VideoOutputMode>(this->parameters.preview.videoOutputMode.get());
							switch (videoOutputMode) {
							case VideoOutputMode::TestPattern:
							{
								if (this->testPattern.isAllocated()) {
									this->testPattern.draw(rectangle);
								}
								break;
							}
							case VideoOutputMode::Data:
							{
								if (this->preview.isAllocated()) {
									this->preview.draw(rectangle);
								}
								break;
							}
							default:
								break;
							}
						}, this);
					};

					videoOutputPin->onDeleteConnection += [this](shared_ptr<System::VideoOutput> videoOutput) {
						this->rebuild();

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
					if (this->decoder) {
						this->decoder->reset();
					}
					
					if(this->previewDirty) {
						this->updatePreview();
					}
					
					if (this->payload) {
						if (this->testPattern.getWidth() != this->payload->getWidth()
							|| this->testPattern.getHeight() != this->payload->getHeight()
							|| this->testPatternBrightness != this->parameters.scan.brightness) {
							this->updateTestPattern();
						}
					}

					if (this->shouldLoadWhenReady && this->hasDecoder()) {
						auto filename = ofFilePath::removeExt(this->getDefaultFilename()) + ".sl";
						this->decoder->loadDataSet(filename);
						this->shouldLoadWhenReady = false;
					}
				}

				//----------
				void Graycode::serialize(Json::Value & json) {
					Utils::Serializable::serialize(json, this->parameters);
					
					if (this->payload) {
						auto & jsonPayload = json["payload"];
						jsonPayload["width"] << this->payload->getWidth();
						jsonPayload["height"] << this->payload->getHeight();
					}

					if (this->decoder) {
						json["hasData"] << true;
						auto filename = ofFilePath::removeExt(this->getDefaultFilename()) + ".sl";
						this->decoder->saveDataSet(filename);
					}
					else {
						json["hasData"] << false;
					}
				}

				//----------
				void Graycode::deserialize(const Json::Value & json) {
					Utils::Serializable::deserialize(json, this->parameters);

					//load payload if there is one and we don't have a videooutput attached and open
					auto videoOutputNode = this->getInput<System::VideoOutput>();
					if (videoOutputNode && videoOutputNode->isWindowOpen()) {
						//use videoOutput to define the payload
						this->rebuild();
					}
					else {
						//load the payload from the file if no window open
						const auto & jsonPayload = json["payload"];
						if (jsonPayload.isMember("width")) {
							uint32_t width, height;
							jsonPayload["width"] >> width;
							jsonPayload["height"] >> height;

							this->encoder.reset();
							this->decoder.reset();
							this->payload = make_unique<ofxGraycode::PayloadGraycode>();
						}
					}

					{
						bool hasData = false;
						json["hasData"] >> hasData;
						if (hasData) {
							this->shouldLoadWhenReady = true;
						}
						else {
							this->shouldLoadWhenReady = false;
						}
					}

					//deal with parameters
					if (this->hasDecoder()) {
						this->decoder->setThreshold(this->parameters.processing.threshold);
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

					//initialise
					this->rebuild();
					if (!this->payload || !this->decoder || !this->encoder) {
						throw(ofxRulr::Exception("Payload, Decoder or Encoder not initialised."));
					}
					this->decoder->setThreshold(this->parameters.processing.threshold);

					//clear the output
					this->message.clear();

					ofHideCursor();

					try {
						Utils::ScopedProcess scopedProcess("Scanning graycode", true, this->payload->getFrameCount());

						while (*this->encoder >> this->message) {
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
								videoOutput->clearFbo(false);
								videoOutput->begin();
								//
								ofPushStyle();
								auto brightness = this->parameters.scan.brightness;
								ofSetColor(brightness);
								this->message.draw(0, 0);
								ofPopStyle();
								//
								videoOutput->end();
								videoOutput->presentFbo();

								auto startWait = ofGetElapsedTimeMillis();
								while (ofGetElapsedTimeMillis() - startWait < this->parameters.scan.captureDelay) {
									ofSleepMillis(1);
									grabber->update();
								}

								for (int i = 0; i < this->parameters.scan.flushFrames; i++) {
									grabber->getFreshFrame();
								}

#ifdef TARGET_OSX
							}
#endif
							auto frame = grabber->getFreshFrame();
							if (!frame) {
								throw(ofxRulr::Exception("Couldn't get fresh frame from camera"));
							}
							* this->decoder << frame->getPixels();
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
					this->rebuild();
				}

				//----------
				bool Graycode::hasData() const {
					if (this->decoder && this->decoder->getDataSet().getHasData()) {
						return true;
					}
					else {
						return false;
					}
				}

				//----------
				bool Graycode::hasDecoder() const {
					if (this->decoder) {
						return true;
					}
					else {
						return false;
					}
				}

				//----------
				ofxGraycode::Decoder & Graycode::getDecoder() const {
					if (this->decoder) {
						return *this->decoder;
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
				void Graycode::rebuild() {
					auto videoOutputNode = this->getInput<System::VideoOutput>();

					if (videoOutputNode && videoOutputNode->isWindowOpen()) {
						this->payload = make_unique<ofxGraycode::PayloadGraycode>();
						this->decoder = make_unique<ofxGraycode::Decoder>();
						this->encoder = make_unique<ofxGraycode::Encoder>();

						this->payload->init(videoOutputNode->getWidth(), videoOutputNode->getHeight());

						this->decoder->init(* this->payload);
						this->encoder->init(* this->payload);
					}
					else {
						this->payload.reset();
						this->encoder.reset();
						this->decoder.reset();
					}

					this->previewDirty = true;
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
							if (this->hasData()) {
								this->decoder->saveDataSet();
								this->decoder->savePreviews();
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
							if (this->payload) {
								return this->payload->getWidth();
							}
							else {
								return (uint32_t)0;
							}
						}));
						inspector->add(new Widgets::LiveValue<unsigned int>("Height", [this]() {
							if (this->payload) {
								return this->payload->getHeight();
							}
							else {
								return (uint32_t)0;
							}
						}));
					}

					inspector->add(new Widgets::Title("Scan camera", Widgets::Title::Level::H2));
					{
						inspector->add(new Widgets::LiveValue<unsigned int>("Width", [this]() {
							if (this->hasDecoder()) {
								return (uint32_t) this->decoder->getWidth();
							}
							else {
								return (uint32_t) 0;
							}
						}));
						inspector->add(new Widgets::LiveValue<unsigned int>("Height", [this]() {
							if (this->hasDecoder()) {
								return (uint32_t) this->decoder->getHeight();
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
					if(this->payload) {
						ofPixels testPatternPixels;
						
						auto width = this->payload->getWidth();
						auto height = this->payload->getHeight();
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
				void Graycode::callbackChangeThreshold(float & value) {
					if (this->decoder) {
						this->decoder->setThreshold(value);
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
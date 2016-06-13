#include "pch_RulrNodes.h"
#include "VideoOutput.h"
#include "ofxRulr/Utils/Constants.h"

#include "ofxCvGui.h"

namespace ofxRulr {
	namespace Nodes {
		namespace System {
#pragma mark MonitorEventChangeListener
			//----------
			MonitorEventChangeListener monitorEventChangeListener = MonitorEventChangeListener();

			//----------
			void monitorCallback(GLFWmonitor * monitor, int connectionState) {
				monitorEventChangeListener.onMonitorChange(monitor);
			}

			//----------
			MonitorEventChangeListener::MonitorEventChangeListener() {
				glfwSetMonitorCallback(&monitorCallback);
			}

#pragma mark MonitorEventChangeListener
			//----------
			VideoOutput::Output::Output(int index, GLFWmonitor * monitor) {
				this->index = index;
				this->monitor = monitor;
				glfwGetMonitorPhysicalSize(monitor, &this->width, &this->height);
				this->name = glfwGetMonitorName(monitor);
			}

#pragma mark VideoOutput

			//----------
			VideoOutput::VideoOutput() {
				RULR_NODE_INIT_LISTENER;
				this->videoMode = nullptr;
			}

			//----------
			void VideoOutput::init() {
				RULR_NODE_UPDATE_LISTENER;
				RULR_NODE_SERIALIZATION_LISTENERS;
				RULR_NODE_INSPECTOR_LISTENER;

				this->onDestroy += [this]() {
					this->setWindowOpen(false);
				};

				auto previewView = make_shared<ofxCvGui::Panels::Draws>(this->fbo);
				previewView->setCaption("Preview");
				previewView->onDraw += [this](ofxCvGui::DrawArguments & args) {
					if (this->mute) {
						ofxCvGui::Utils::drawText("MUTE [SPACE]", args.localBounds);
					}
					if (!this->window) {
						ofxCvGui::Utils::drawText("CLOSED", args.localBounds);
					}
				};

				this->monitorSelectionView = MAKE(ofxCvGui::Panels::ElementHost);
				this->monitorSelectionView->getElementGroup()->onBoundsChange += [this](ofxCvGui::BoundsChangeArguments & args) {
					//arrange buttons and subview for fbo

					auto elements = this->monitorSelectionView->getElementGroup()->getElements();
					auto blockWidth = args.localBounds.width / this->videoOutputs.size();

					float maxButtonHeight = 0;
					int index = 0;
					for (auto element : elements) {
						//currently we set them all to have the same width but this could change
						auto buttonWidth = blockWidth - 20;
						float buttonHeight;

						const auto & videoOutput = this->videoOutputs[index];

						//set the height to match the aspect ratio
						buttonHeight = buttonWidth * (float)videoOutput.height / (float)videoOutput.width;
						//and clamp it incase it was too tall
						buttonHeight = ofClamp(buttonHeight, 0.0f, args.localBounds.getHeight() - 20);

						element->setBounds(ofRectangle(blockWidth * index + 10, 10, blockWidth - 20, buttonHeight));

						//note the maximum button height
						if (buttonHeight > maxButtonHeight) {
							maxButtonHeight = buttonHeight;
						}

						index++;
					}
				};
				auto groupView = make_shared<ofxCvGui::Panels::Groups::Grid>();
				groupView->add(previewView);
				groupView->add(this->monitorSelectionView);
				this->view = groupView;

				this->refreshMonitors();

				this->onDrawOutput += [this](ofRectangle & outputRect) {
					switch (this->testPattern.get()) {
					case 1:
					{
						// GRID
						ofPushStyle();
						{
							ofNoFill();
							ofSetLineWidth(1.0f);
							auto xStep = (this->getWidth() - 1) / 4;
							auto yStep = (this->getHeight() - 1) / 4;
							for (int i = 0; i < 5; i++) {
								ofSetColor((i % 2 == 0) ? 255 : 100);
								auto x = xStep * i + 0.5f;
								auto y = yStep * i + 0.5f;
								ofDrawLine(x, 0, x, this->getHeight());
								ofDrawLine(0, y, this->getWidth(), y);
							}
							stringstream text;
							text << this->getName() << endl;
							text << this->getWidth() << "x" << this->getHeight() << endl;
							text << "Output " << this->videoOutputSelection << " [/" << this->videoOutputs.size() << "]" << endl;
							if (this->splitHorizontal > 1 || this->splitVertical > 1) {
								text << "Split portion " << this->splitUseIndex << " [" << this->splitHorizontal << "x" << this->splitVertical << "]";
							}
							else {
								text << "No split";
							}
							ofDrawBitmapString(text.str(), xStep * 2 + 10, yStep * 2 - 50);
						}
						ofPopStyle();
						break;
					}
					case 2:
						// WHITE
						ofClear(255, 255);
						break;
					default:
						break;
					}
				};

				this->useFullScreenMode.set("Use real fullscreen", false);
				this->splitHorizontal.set("Split Horizontal", 1, 1, 8);
				this->splitVertical.set("Split Vertical", 1, 1, 8);
				this->splitUseIndex.set("Selected portion", 0, 0, 8);
				this->testPattern.set("Test Pattern", 1);
				this->mute.set("Mute", false);

				this->splitHorizontal.addListener(this, &VideoOutput::callbackChangeSplit);
				this->splitVertical.addListener(this, &VideoOutput::callbackChangeSplit);
				this->splitUseIndex.addListener(this, &VideoOutput::callbackChangeSplit);
				this->useFullScreenMode.addListener(this, &VideoOutput::callbackChangeFullscreenMode);

				monitorEventChangeListener.onMonitorChange += [this](GLFWmonitor *) {
					this->needsMonitorRefresh = true;
				};

				this->width = 1024.0f;
				this->height = 768.0f;
				this->videoOutputSelection = 0;
				this->scissorWasEnabled = false;
			}

			//----------
			string VideoOutput::getTypeName() const {
				return "System::VideoOutput";
			}

			//----------
			void VideoOutput::serialize(Json::Value & json) {
				//we cache the width and height in case anybody wants to know what it is
				json["width"] = this->width;
				json["height"] = this->height;
				json["monitorSelection"] = this->videoOutputSelection;
				ofxRulr::Utils::Serializable::serialize(json, this->splitHorizontal);
				ofxRulr::Utils::Serializable::serialize(json, this->splitVertical);
				ofxRulr::Utils::Serializable::serialize(json, this->splitUseIndex);
				ofxRulr::Utils::Serializable::serialize(json, this->testPattern);
				ofxRulr::Utils::Serializable::serialize(json, this->mute);
			}

			//----------
			void VideoOutput::deserialize(const Json::Value & json) {
				this->width = json["width"].asInt();
				this->height = json["height"].asInt();
				this->setVideoOutputSelection(json["monitorSelection"].asInt());
				ofxRulr::Utils::Serializable::deserialize(json, this->splitHorizontal);
				ofxRulr::Utils::Serializable::deserialize(json, this->splitVertical);
				ofxRulr::Utils::Serializable::deserialize(json, this->splitUseIndex);
				ofxRulr::Utils::Serializable::deserialize(json, this->testPattern);
				ofxRulr::Utils::Serializable::deserialize(json, this->mute);
			}

			//----------
			ofxCvGui::PanelPtr VideoOutput::getPanel() {
				return this->view;
			}

			//----------
			void VideoOutput::populateInspector(ofxCvGui::InspectArguments & inspectArguments) {
				auto inspector = inspectArguments.inspector;
				
				inspector->add(MAKE(ofxCvGui::Widgets::Button, "Refresh monitors", [this]() {
					this->refreshMonitors();
				}));

				auto showWindowToggle = inspector->addToggle("Show window", [this]() {
					return this->isWindowOpen();
				}, [this](bool value) {
					this->setWindowOpen(value);
				});
				{
					showWindowToggle->setHotKey(OF_KEY_RETURN);
					showWindowToggle->setHeight(100.0f);
					inspector->add(showWindowToggle);
				}

				inspector->add(make_shared<ofxCvGui::Widgets::LiveValue<string>>("Monitor selection", [this]() {
					return ofToString(this->videoOutputSelection) + " of " + ofToString(this->videoOutputs.size());
				}));

				inspector->add(MAKE(ofxCvGui::Widgets::LiveValue<string>, "Window status", [this]() {
					if (!this->window) {
						return string("closed");
					}
					else {
						return string("open");
					}
				}));

				inspector->addToggle(this->useFullScreenMode);

				inspector->add(MAKE(ofxCvGui::Widgets::LiveValue<ofVec2f>, "Output Size", [this]() {
					return ofVec2f(this->getWidth(), this->getHeight());
				}));

				inspector->add(MAKE(ofxCvGui::Widgets::Title, "Split output", ofxCvGui::Widgets::Title::Level::H3));
				{
					auto splitHorizontalSlider = MAKE(ofxCvGui::Widgets::Slider, this->splitHorizontal);
					auto splitVerticalSlider = MAKE(ofxCvGui::Widgets::Slider, this->splitVertical);
					auto splitUseIndexSlider = MAKE(ofxCvGui::Widgets::Slider, this->splitUseIndex);
					splitHorizontalSlider->addIntValidator();
					splitVerticalSlider->addIntValidator();
					splitUseIndexSlider->addIntValidator();
					inspector->add(splitHorizontalSlider);
					inspector->add(splitVerticalSlider);
					inspector->add(splitUseIndexSlider);
				}

				auto testPatternSelector = make_shared<ofxCvGui::Widgets::MultipleChoice>("Test pattern");
				{
					testPatternSelector->addOption("None");
					testPatternSelector->addOption("Grid");
					testPatternSelector->addOption("White");
					testPatternSelector->setSelection(this->testPattern.get());
					testPatternSelector->onValueChange += [this](const int & selection) {
						this->testPattern = selection;
					};
					inspector->add(testPatternSelector);
				}
				
				auto muteToggle = inspector->addToggle(this->mute);
				{
					muteToggle->setHotKey(' ');
					muteToggle->setHeight(100.0f);
				}
			}

			//----------
			void VideoOutput::update() {
				if (this->needsMonitorRefresh) {
					this->refreshMonitors();
				}

				if (this->splitHorizontal < 1) {
					this->splitHorizontal = 1;
				}
				if (this->splitVertical < 1) {
					this->splitVertical = 1;
				}
				if (this->splitUseIndex >= this->splitHorizontal * this->splitVertical) {
					this->splitUseIndex.set(ofClamp(this->splitUseIndex, 0, this->splitHorizontal * this->splitVertical - 1));
				}

				//show and then clear the window
				if (this->window) {
					this->presentFbo();
					this->clearFbo(true);
				}
				
				//set the window title
				if(this->window && this->windowTitle != this->getName()) {
					this->window->setWindowTitle(this->getName());
					this->windowTitle = this->getName();
				}
			}

			//----------
			float VideoOutput::getWidth() const {
				return this->width;
			}

			//----------
			float VideoOutput::getHeight() const {
				return this->height;
			}

			//----------
			ofRectangle VideoOutput::getSize() const {
				return ofRectangle(0.0f, 0.0f, this->width, this->height);
			}

			//----------
			int VideoOutput::getVideoOutputSelection() const {
				return this->videoOutputSelection;
			}

			//----------
			const VideoOutput::Output & VideoOutput::getVideoOutputSelectionObject() const {
				return this->videoOutputs[this->getVideoOutputSelection()];
			}

			//----------
			void VideoOutput::setVideoOutputSelection(int videoOutputSelection){
				bool windowWasOpen = this->isWindowOpen();
				this->setWindowOpen(false);

				this->videoOutputSelection = videoOutputSelection;
				this->calculateSplit();

				if (windowWasOpen) {
					this->setWindowOpen(true);
				}
			}

			//----------
			int VideoOutput::getVideoOutputCount() const {
				return (int) this->videoOutputs.size();
			}

			//----------
			ofRectangle VideoOutput::getRectangleInCombinedOutput() const {
				float x = (int) this->splitUseIndex % (int) this->splitHorizontal * this->getWidth();
				float y = (int) this->splitUseIndex / (int) this->splitHorizontal * this->getHeight();
				return ofRectangle(x, y, this->getWidth(), this->getHeight());
			}

			//----------
			void VideoOutput::applyNormalisedSplitViewTransform() const {
				ofVec2f tanslation = ofVec2f(-1, -1) + 1 / ofVec2f(this->splitHorizontal, this->splitVertical);
				tanslation += ofVec2f(2.0f / this->splitHorizontal, 2.0f / this->splitVertical) *
					ofVec2f((int) this->splitUseIndex % (int) this->splitHorizontal, (int) this->splitUseIndex / (int) this->splitHorizontal);
				ofTranslate(tanslation);
				ofScale(1.0f / this->splitHorizontal, 1.0f / this->splitVertical, 1.0f);
			}

			//----------
			void VideoOutput::setWindowOpen(bool windowOpen) {
				if (windowOpen) {
					this->createWindow();
				}
				else {
					this->destroyWindow();
				}
			}

			//----------
			bool VideoOutput::isWindowOpen() const {
				return this->window != nullptr;
			}

			//----------
			void VideoOutput::setMute(bool mute) {
				this->mute = mute;
			}

			//----------
			bool VideoOutput::getMute() const {
				return this->mute;
			}

			//----------
			ofFbo & VideoOutput::getFbo() {
				return this->fbo;
			}

			//----------
			GLFWwindow * VideoOutput::getWindow() const {
				return this->window->getGLFWWindow();
			}

			//----------
			void VideoOutput::clearFbo(bool callDrawListeners) {
				this->begin();
				ofClear(0, 255);

				if (callDrawListeners) {
					auto size = this->getSize();
					this->onDrawOutput.notifyListeners(size);
				}

				this->end();
			}

			//----------
			void VideoOutput::begin() {
				this->scissorWasEnabled = ofxCvGui::Utils::ScissorManager::X().getScissorEnabled();
				ofxCvGui::Utils::ScissorManager::X().setScissorEnabled(false); 
				this->fbo.begin(true);
			}

			//----------
			void VideoOutput::end() {
				this->fbo.end();
				if (this->scissorWasEnabled) {
					ofxCvGui::Utils::ScissorManager::X().setScissorEnabled(true);
				}
			}
			
			//----------
			void VideoOutput::presentFbo() {
				if (!this->window) {
					return;
				}

				//remove any scissor
				auto scissorEnabled = ofxCvGui::Utils::ScissorManager::X().getScissorEnabled();
				ofxCvGui::Utils::ScissorManager::X().setScissorEnabled(false);

				//Open and draw to the window
				auto appWindow = ofGetCurrentWindow();
				ofGetMainLoop()->setCurrentWindow(this->window);
				this->window->makeCurrent();
				this->window->update();
				this->window->renderer()->startRender();
				{
					ofPushView();
					{
						//ofClear(0, 255);
						ofSetupScreenOrtho();
						
 						ofDrawCircle(50, 50, 50);
 						ofDrawBitmapStringHighlight("this is the client window", 20, 20);
 						this->fbo.draw(20, 50);

						//clear the entire video output
						ofClear(0, 0);

						//draw the fbo if mute is disabled
						if (!this->mute) {
							this->fbo.draw(0, 0);
						}
					}
					ofPopView();
				}
				this->window->swapBuffers();
				this->window->renderer()->finishRender();
				ofGetMainLoop()->setCurrentWindow(appWindow);
				appWindow->makeCurrent();

				if (scissorEnabled) {
					ofxCvGui::Utils::ScissorManager::X().setScissorEnabled(true);
				}
			}

			//----------
			void VideoOutput::refreshMonitors() {
				int monitorCount;
				auto monitors = glfwGetMonitors(&monitorCount);

				//fill videoOutputs
				this->videoOutputs.clear();
				for (int i = 0; i < monitorCount; i++) {
					const auto videoOutput = Output(i, monitors[i]);
					this->videoOutputs.push_back(videoOutput);
				}

				//build gui
				this->monitorSelectionView->getElementGroup()->clear();
				for (auto & videoOutput : this->videoOutputs) {
					auto selectButton = MAKE(ofxCvGui::Element);

					selectButton->onDraw += [this, selectButton, &videoOutput](ofxCvGui::DrawArguments & args) {
						bool selected = this->videoOutputSelection == videoOutput.index;

						//modified from ofxCvGui::Widgets::Toggle::draw()

						auto & font = ofxAssets::Register::X().getFont(ofxCvGui::getDefaultTypeface(), 12);

						ofPushStyle();
						{
							//fill
							if (selected ^ selectButton->isMouseDown()) {
								if (this->window) {
									ofSetColor(80, 120, 80);
								}
								else {
									ofSetColor(80);
								}
							}
							else {
								ofSetColor(50);
							}
							ofFill();
							const auto radius = 5.0f;
							ofDrawRectRounded(args.localBounds, radius, radius, radius, radius);

							//outline
							if (selectButton->isMouseOver()) {
								ofNoFill();
								ofSetColor(!selected ? 80 : 50);
								ofDrawRectRounded(args.localBounds, radius, radius, radius, radius);
							}

							//split
							if (selected) {
								ofPushStyle();
								{
									ofSetLineWidth(1.0f);
									ofSetColor(50);
									for (int borderIndex = 0; borderIndex < this->splitHorizontal - 1; borderIndex++) {
										const float x = selectButton->getWidth() / this->splitHorizontal * (borderIndex + 1);
										ofDrawLine(x, 0, x, args.localBounds.height);
									}
									for (int borderIndex = 0; borderIndex < this->splitVertical - 1; borderIndex++) {
										const float y = selectButton->getHeight() / this->splitVertical * (borderIndex + 1);
										ofDrawLine(0, y, args.localBounds.width, y);
									}
								}
								ofPopStyle();
							}

							ofSetColor(255);
							const auto textBounds = font.getStringBoundingBox(videoOutput.name, 0, 0);
							font.drawString(videoOutput.name, (int)((selectButton->getWidth() - textBounds.width) / 2.0f), (int)((selectButton->getHeight() + textBounds.height) / 2.0f));
						}
						ofPopStyle();
					};

					selectButton->onMouseReleased += [this, &videoOutput](ofxCvGui::MouseArguments & args) {
						this->setVideoOutputSelection(videoOutput.index);
					};

					//turn the scissor on
					selectButton->setScissor(true);

					this->monitorSelectionView->getElementGroup()->add(selectButton);
				}

				this->needsMonitorRefresh = false;
			}

			//----------
			void VideoOutput::createWindow() {
				this->destroyWindow();

				this->refreshMonitors();

				//check we have a valid index selected
				if ((unsigned int) this->videoOutputSelection < this->videoOutputs.size()) {
					//--
					//calculate window shape
					//--
					//
					const auto & videoOutput = this->getVideoOutputSelectionObject();
					int x, y;

					glfwGetMonitorPos(videoOutput.monitor, &x, &y);

					x += this->width * ((int) this->splitUseIndex.get() % (int) this->splitHorizontal.get());
					y += this->height * ((int) this->splitUseIndex.get() / (int) this->splitHorizontal.get());

					this->videoMode = glfwGetVideoMode(videoOutput.monitor);
					this->calculateSplit();
					//
					//--



					//--
					//create window
					//--
					//
					{
						auto appWindow = ofGetCurrentWindow();
						ofGLFWWindowSettings windowSettings;
						windowSettings.shareContextWith = appWindow;

						auto hasSplit = this->splitHorizontal > 1 || this->splitVertical > 1;
						bool useFullScreen = !hasSplit && this->useFullScreenMode;
						if (useFullScreen) {
							windowSettings.monitor = videoOutput.index;
							windowSettings.width = this->width;
							windowSettings.height = this->height;
							windowSettings.windowMode = OF_GAME_MODE;
						}
						else {
							windowSettings.decorated = false;
							windowSettings.resizable = false;
							windowSettings.windowMode = OF_WINDOW;
							windowSettings.width = this->width;
							windowSettings.height = this->height;
							windowSettings.setPosition(ofVec2f(x, y));
						}

						this->window = make_shared<ofAppGLFWWindow>();
						this->window->setup(windowSettings);
						this->window->update();

						appWindow->makeCurrent();
					}
					//
					//--



					//--
					//allocate fbo to match the window
					//--
					//
					{
						ofFbo::Settings fboSettings;
						fboSettings.width = this->width;
						fboSettings.height = this->height;
						fboSettings.minFilter = GL_NEAREST;
						fboSettings.maxFilter = GL_NEAREST;
						this->fbo.allocate(fboSettings);
					}
					//
					//--
				}
			}

			//----------
			void VideoOutput::destroyWindow() {
				this->window.reset();
				this->fbo.clear();
			}

			//----------
			void VideoOutput::calculateSplit() {
				if (!this->window) {
					if (this->videoOutputSelection < this->videoOutputs.size()) {
						this->videoMode = glfwGetVideoMode(this->videoOutputs[this->videoOutputSelection].monitor);
					}
				}

				//make sure we don't have any erroneous settings
				if (this->splitHorizontal.get() < 1) {
					this->splitHorizontal.disableEvents();
					this->splitHorizontal = 1;
					this->splitHorizontal.enableEvents();
				}

				if (this->splitVertical.get() < 1) {
					this->splitVertical.disableEvents();
					this->splitVertical = 1;
					this->splitVertical.enableEvents();
				}

				if (this->videoMode) {
					this->width = videoMode->width / this->splitHorizontal;
					this->height = videoMode->height / this->splitVertical;

					const auto splitCount = this->splitHorizontal * this->splitVertical;
					this->splitUseIndex.setMax(splitCount - 1);

					if (this->isWindowOpen()) {
						this->setWindowOpen(true);
					}
				}
			}

			//----------
			void VideoOutput::callbackChangeSplit(float &) {
				this->calculateSplit();
				if (this->isWindowOpen()) {
					this->createWindow();
				}
			}

			//----------
			void VideoOutput::callbackChangeFullscreenMode(bool &) {
				if (this->isWindowOpen()) {
					this->createWindow();
				}
			}
		}
	}
}
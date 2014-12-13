#include "VideoOutput.h"

#include "ofxCvGui.h"

namespace ofxDigitalEmulsion {
	namespace Device {
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
			glfwGetMonitorPhysicalSize(monitor, & this->width, & this->height);
			this->name = glfwGetMonitorName(monitor);
		}
#pragma mark VideoOutput

		//----------
		VideoOutput::VideoOutput() {
			this->videoOutputSelection = 0;

			this->view = MAKE(ofxCvGui::Panels::ElementHost);
			this->refreshMonitors();

			this->view->getElementGroup()->onBoundsChange += [this](ofxCvGui::BoundsChangeArguments & args) {
				//arrange buttons and subview for fbo

				auto elements = this->view->getElementGroup()->getElements();
				auto blockWidth = args.localBounds.width / this->videoOutputs.size();

				int index = 0;
				float maxButtonHeight = 0;
				for (auto element : elements) {
					if (index < this->getVideoOutputCount()) {
						//it's a monitor
						
						//currently we set them all to have the same width but this could change
						auto buttonWidth = blockWidth - 20;
						float buttonHeight;

						const auto & videoOutput = this->videoOutputs[index];

						//set the height to match the aspect ratio
						buttonHeight = buttonWidth * (float)videoOutput.height / (float)videoOutput.width;
						//and clamp it incase it was too tall
						buttonHeight = ofClamp(buttonHeight, 0.0f, args.localBounds.getHeight() - 50);
						
						element->setBounds(ofRectangle(blockWidth * index + 10, 70, blockWidth - 20, buttonHeight));

						//note the maximum button height
						if (buttonHeight > maxButtonHeight) {
							maxButtonHeight = buttonHeight;
						}
					}
					else {
						//it's the fbo
						element->setBounds(ofRectangle(10, maxButtonHeight + 80, args.localBounds.width - 20, 
							MAX(0, args.localBounds.height - (maxButtonHeight + 80 + 20))
							));
					}
					index++;
				}
			};

			this->onDrawOutput += [this](ofRectangle &) {
				switch (this->testPattern.get()) {
				case 1:
				{
					// GRID
					const auto outputRect = this->getSize();
					ofPushStyle();
					ofNoFill();
					ofSetLineWidth(1.0f);
					ofRect(outputRect);
					ofLine(outputRect.getCenter() - ofVec2f(outputRect.width / 2.0f, 0.0f), outputRect.getCenter() + ofVec2f(outputRect.width / 2.0f, 0.0f));
					ofLine(outputRect.getCenter() - ofVec2f(0.0f, outputRect.height / 2.0f), outputRect.getCenter() + ofVec2f(0.0f, outputRect.height / 2.0f));
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

			this->showWindow.set("Show window", false);
			this->testPattern.set("Test Pattern", 0);
			this->splitHorizontal.set("Split Horizontal", 1, 1, 8);
			this->splitVertical.set("Split Vertical", 1, 1, 8);
			this->splitUseIndex.set("Selected portion", 0, 0, 8);

			this->splitHorizontal.addListener(this, &VideoOutput::callbackChangeSplit);
			this->splitVertical.addListener(this, &VideoOutput::callbackChangeSplit);

			this->window = nullptr;
			this->videoMode = nullptr;

			monitorEventChangeListener.onMonitorChange += [this](GLFWmonitor *) {
				this->needsMonitorRefresh = true;
			};
			this->width = 1024.0f;
			this->height = 768.0f;
		}

		//----------
		void VideoOutput::init() {
		}

		//----------
		string VideoOutput::getTypeName() const {
			return "Device::VideoOutput";
		}

		//----------
		void VideoOutput::serialize(Json::Value & json) {
			//we cache the width and height in case anybody wants to know what it is
			json["width"] = this->width;
			json["height"] = this->height;
			json["monitorSelection"] = this->videoOutputSelection;
			ofxDigitalEmulsion::Utils::Serializable::serialize(this->splitHorizontal, json);
			ofxDigitalEmulsion::Utils::Serializable::serialize(this->splitVertical, json);
			ofxDigitalEmulsion::Utils::Serializable::serialize(this->splitUseIndex, json);
			ofxDigitalEmulsion::Utils::Serializable::serialize(this->testPattern, json);
		}

		//----------
		void VideoOutput::deserialize(const Json::Value & json) {
			this->width = json["width"].asInt();
			this->height = json["height"].asInt();
			this->setVideoOutputSelection(json["monitorSelection"].asInt());
			ofxDigitalEmulsion::Utils::Serializable::deserialize(this->splitHorizontal, json);
			ofxDigitalEmulsion::Utils::Serializable::deserialize(this->splitVertical, json);
			ofxDigitalEmulsion::Utils::Serializable::deserialize(this->splitUseIndex, json);
			ofxDigitalEmulsion::Utils::Serializable::deserialize(this->testPattern, json);
		}

		//----------
		ofxCvGui::PanelPtr VideoOutput::getView() {
			return this->view;
		}

		//----------
		void VideoOutput::populateInspector2(ofxCvGui::ElementGroupPtr inspector) {
			inspector->add(MAKE(ofxCvGui::Widgets::Button, "Refresh monitors", [this]() {
				this->refreshMonitors();
			}));

			auto showWindowToggle = MAKE(ofxCvGui::Widgets::Toggle, this->showWindow);
			showWindowToggle->setHeight(100.0f);
			inspector->add(showWindowToggle);
			showWindowToggle->onValueChange += [this](ofParameter<bool> & value) {
				this->setWindowOpen(value);
			};

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

			inspector->add(MAKE(ofxCvGui::Widgets::LiveValue<ofVec2f>, "Output Size", [this]() {
				return ofVec2f(this->getWidth(), this->getHeight());
			}));

			inspector->add(MAKE(ofxCvGui::Widgets::Title, "Split output", ofxCvGui::Widgets::Title::Level::H3));
			auto splitHorizontalSlider = MAKE(ofxCvGui::Widgets::Slider, this->splitHorizontal);
			auto splitVerticalSlider = MAKE(ofxCvGui::Widgets::Slider, this->splitVertical);
			auto splitUseIndexSlider = MAKE(ofxCvGui::Widgets::Slider, this->splitUseIndex);
			splitHorizontalSlider->addIntValidator();
			splitVerticalSlider->addIntValidator();
			splitUseIndexSlider->addIntValidator();
			inspector->add(splitHorizontalSlider);
			inspector->add(splitVerticalSlider);
			inspector->add(splitUseIndexSlider);

			auto testPatternSelector = make_shared<ofxCvGui::Widgets::MultipleChoice>("Test pattern");
			testPatternSelector->addOption("None");
			testPatternSelector->addOption("Grid");
			testPatternSelector->addOption("White");
			testPatternSelector->setSelection(this->testPattern.get());
			testPatternSelector->onSelectionChange += [this](const int & selection){
				this->testPattern = selection;
			};
			inspector->add(testPatternSelector);
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

			this->presentFbo();
			this->clearFbo(true);
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
			return this->window;
		}

		//----------
		ofFbo & VideoOutput::getFbo() {
			return this->fbo;
		}

		//----------
		GLFWwindow * VideoOutput::getWindow() const {
			return this->window;
		}

		//----------
		void VideoOutput::clearFbo(bool callDrawListeners) {
			this->fbo.begin();
			ofClear(0, 255);

			if (callDrawListeners) {
				auto size = this->getSize();
				this->onDrawOutput.notifyListeners(size);
			}

			this->fbo.end();
		}

		//----------
		void VideoOutput::begin() {
			this->fbo.begin();
		}

		//----------
		void VideoOutput::end() {
			this->fbo.end();
		}

		//----------
		void VideoOutput::presentFbo() {
			if (!this->window) {
				return;
			}
			//Open and draw to the window
			auto mainWindow = glfwGetCurrentContext();
			glfwMakeContextCurrent(this->window);

			//clear the entire video output
			glViewport(0, 0, this->videoMode->width, this->videoMode->height);
			ofClear(0, 0);

			//set the viewport to the right projector
			const auto viewport = this->getRectangleInCombinedOutput();
			GLint viewportCached[4];
			glGetIntegerv(GL_VIEWPORT, viewportCached);
			glViewport(viewport.x, viewport.y, viewport.width, viewport.height); //<--needs checking for vertical splits (might be upside down)

			//set the drawing matrices to normalised coordinates
			ofSetMatrixMode(OF_MATRIX_PROJECTION);
			ofLoadIdentityMatrix();
			ofSetMatrixMode(OF_MATRIX_MODELVIEW);
			ofLoadIdentityMatrix();
			this->fbo.draw(-1, -1, 2, 2);

			glfwSwapBuffers(this->window);
			glFlush();

			//return to main window
			glfwMakeContextCurrent(mainWindow);
			glViewport(viewportCached[0], viewportCached[1], viewportCached[2], viewportCached[3]);
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
			this->view->getElementGroup()->clear();
			for (auto & videoOutput : this->videoOutputs) {
				auto selectButton = MAKE(ofxCvGui::Element);

				selectButton->onDraw += [this, selectButton, &videoOutput](ofxCvGui::DrawArguments & args) {
					bool selected = this->videoOutputSelection == videoOutput.index;

					//modified from ofxCvGui::Widgets::Toggle::draw()

					auto & font = ofxAssets::AssetRegister.getFont(ofxCvGui::defaultTypeface, 12);

					ofPushStyle();

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
					ofRectRounded(args.localBounds, radius, radius, radius, radius);

					//outline
					if (selectButton->isMouseOver()) {
						ofNoFill();
						ofSetColor(!selected ? 80 : 50);
						ofRectRounded(args.localBounds, radius, radius, radius, radius);
					}

					//split
					if (selected) {
						ofPushStyle();
						ofSetLineWidth(1.0f);
						ofSetColor(50);
						for (int borderIndex = 0; borderIndex < this->splitHorizontal - 1; borderIndex++) {
							const float x = selectButton->getWidth() / this->splitHorizontal * (borderIndex + 1);
							ofLine(x, 0, x, args.localBounds.height);
						}
						for (int borderIndex = 0; borderIndex < this->splitVertical - 1; borderIndex++) {
							const float y = selectButton->getHeight() / this->splitVertical * (borderIndex + 1);
							ofLine(0, y, args.localBounds.width, y);
						}
						ofPopStyle();
					}

					ofSetColor(255);
					const auto textBounds = font.getStringBoundingBox(videoOutput.name, 0, 0);
					font.drawString(videoOutput.name, (int)((selectButton->getWidth() - textBounds.width) / 2.0f), (int)((selectButton->getHeight() + textBounds.height) / 2.0f));

					ofPopStyle();
				};

				selectButton->onMouseReleased += [this, &videoOutput](ofxCvGui::MouseArguments & args) {
					this->setVideoOutputSelection(videoOutput.index);
				};

				//turn the scissor on
				selectButton->setScissor(true);

				this->view->getElementGroup()->add(selectButton);
			}

			auto subViewForFbo = make_shared<ofxCvGui::Panels::Draws>(this->fbo);
			subViewForFbo->setCaption("Output");
			this->view->getElementGroup()->add(subViewForFbo);

			this->view->getElementGroup()->arrange();

			this->needsMonitorRefresh = false;
		}

		//----------
		void VideoOutput::createWindow() {
			this->destroyWindow();

			this->refreshMonitors();

			//check we have a valid index selected
			if ((unsigned int) this->videoOutputSelection < this->videoOutputs.size()) {
				const auto & videoOutput = this->getVideoOutputSelectionObject();
				int x, y;

				glfwGetMonitorPos(videoOutput.monitor, &x, &y);

				this->videoMode = glfwGetVideoMode(videoOutput.monitor);
				this->calculateSplit();

				glfwWindowHint(GLFW_DECORATED, GL_FALSE);
				this->window = glfwCreateWindow(this->videoMode->width, this->videoMode->height, this->getName().c_str(), NULL, glfwGetCurrentContext());
				glfwWindowHint(GLFW_DECORATED, GL_TRUE);
				glfwSetWindowPos(this->window, x, y);
			}
			else {
				this->showWindow = false;
			}
		}

		//----------
		void VideoOutput::destroyWindow() {
			if (this->window) {
				glfwDestroyWindow(this->window);
				this->window = nullptr;
			}
		}

		//----------
		void VideoOutput::calculateSplit() {
			if (!this->window) {
				if (this->videoOutputSelection < this->videoOutputs.size()) {
					this->videoMode = glfwGetVideoMode(this->videoOutputs[this->videoOutputSelection].monitor);
				}
			}
			if (this->videoMode) {
				this->width = videoMode->width / this->splitHorizontal;
				this->height = videoMode->height / this->splitVertical;

				const auto splitCount = this->splitHorizontal * this->splitVertical;
				this->splitUseIndex.setMax(splitCount - 1);

				this->fbo.allocate(videoMode->width, videoMode->height, GL_RGBA);
			}
		}

		//----------
		void VideoOutput::callbackChangeSplit(float &) {
			this->calculateSplit();
		}
	}
}
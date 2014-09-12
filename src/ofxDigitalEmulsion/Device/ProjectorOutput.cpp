#include "ProjectorOutput.h"

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

#pragma mark ProjectorOutput

		//----------
		ProjectorOutput::ProjectorOutput() {
			this->monitorCount = 0;
			this->monitorSelection = 0;
			this->view = MAKE(ofxCvGui::Panels::ElementHost);
			this->refreshMonitors();
			this->view->getElementGroup()->onBoundsChange += [this](ofxCvGui::BoundsChangeArguments & args) {
				auto elements = this->view->getElementGroup()->getElements();
				auto blockWidth = args.localBounds.width / this->monitorCount;
				int index = 0;
				float maxButtonHeight = 0;
				for (auto element : elements) {
					if (index < this->monitorCount) {
						//it's a monitor
						auto buttonWidth = blockWidth - 20;
						float buttonHeight;
						if (this->monitors && index < this->monitorCount) {
							const GLFWvidmode * monitorVideoMode = glfwGetVideoMode(this->monitors[index]);
							buttonHeight = buttonWidth * (float)monitorVideoMode->height / (float)monitorVideoMode->width;
						}
						else {
							args.localBounds.getHeight();
						}
						buttonHeight = ofClamp(buttonHeight, 0.0f, args.localBounds.getHeight() - 20);
						if (buttonHeight > maxButtonHeight) {
							maxButtonHeight = buttonHeight;
						}
						element->setBounds(ofRectangle(blockWidth * index + 10, 70, blockWidth - 20, buttonHeight));
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

			this->showWindow.set("Show window", false);
			this->splitHorizontal.set("Split Horizontal", 1, 1, 8);
			this->splitVertical.set("Split Vertical", 1, 1, 8);
			this->splitUseIndex.set("Selected portion", 0, 0, 8);

			this->splitHorizontal.addListener(this, &ProjectorOutput::callbackChangeSplit);
			this->splitVertical.addListener(this, &ProjectorOutput::callbackChangeSplit);

			this->window = nullptr;
			this->videoMode = nullptr;

			monitorEventChangeListener.onMonitorChange += [this](GLFWmonitor *) {
				this->needsMonitorRefresh = true;
			};
			this->width = 1024.0f;
			this->height = 768.0f;
		}

		//----------
		string ProjectorOutput::getTypeName() const {
			return "ProjectorOutput";
		}

		//----------
		void ProjectorOutput::serialize(Json::Value & json) {
			json["width"] = this->width;
			json["height"] = this->height;
			json["monitorSelection"] = this->monitorSelection;
			ofxDigitalEmulsion::Utils::Serializable::serialize(this->splitHorizontal, json);
			ofxDigitalEmulsion::Utils::Serializable::serialize(this->splitVertical, json);
			ofxDigitalEmulsion::Utils::Serializable::serialize(this->splitUseIndex, json);
		}

		//----------
		void ProjectorOutput::deserialize(const Json::Value & json) {
			this->width = json["width"].asInt();
			this->height = json["height"].asInt();
			this->monitorSelection = json["monitorSelection"].asInt();
			ofxDigitalEmulsion::Utils::Serializable::deserialize(this->splitHorizontal, json);
			ofxDigitalEmulsion::Utils::Serializable::deserialize(this->splitVertical, json);
			ofxDigitalEmulsion::Utils::Serializable::deserialize(this->splitUseIndex, json);
		}

		//----------
		ofxCvGui::PanelPtr ProjectorOutput::getView() {
			return this->view;
		}

		//----------
		void ProjectorOutput::refreshMonitors() {
			this->monitors = glfwGetMonitors(&this->monitorCount);

			this->view->getElementGroup()->clear();
			for (int i = 0; i<this->monitorCount; i++) {
				auto selectButton = MAKE(ofxCvGui::Element);
				selectButton->onDraw += [this, i, selectButton](ofxCvGui::DrawArguments & args) {
					bool selected = this->monitorSelection == i;

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
					string caption = "Output " + ofToString(i);
					const auto textBounds = font.getStringBoundingBox(caption, 0, 0);
					font.drawString(caption, (int)((selectButton->getWidth() - textBounds.width) / 2.0f), (int)((selectButton->getHeight() + textBounds.height) / 2.0f));

					ofPopStyle();
				};

				selectButton->onMouseReleased += [this, i](ofxCvGui::MouseArguments & args) {
					if (args.isLocal()) {
						this->monitorSelection = i;
						this->calculateSplit();
						if (this->window) {
							this->createWindow();
						}
					}
				};
				this->view->getElementGroup()->add(selectButton);
			}
			this->view->getElementGroup()->addBlank()->onDraw += [this](ofxCvGui::DrawArguments & args) {
				this->fbo.draw(args.localBounds);
			};
			this->view->getElementGroup()->arrange();

			this->needsMonitorRefresh = false;
		}

		//----------
		void ProjectorOutput::populateInspector2(ofxCvGui::ElementGroupPtr inspector) {
			inspector->add(MAKE(ofxCvGui::Widgets::Button, "Refresh monitors", [this]() {
				this->refreshMonitors();
			}));

			auto showWindowToggle = MAKE(ofxCvGui::Widgets::Toggle, this->showWindow);
			showWindowToggle->setHeight(100.0f);
			inspector->add(showWindowToggle);
			showWindowToggle->onValueChange += [this](ofParameter<bool> & value) {
				this->setWindowOpen(value);
			};

			inspector->add(MAKE(ofxCvGui::Widgets::LiveValue<string>, "Monitor selection", [this]() {
				return ofToString(this->monitorSelection) + " of " + ofToString(this->monitorCount);
			}));

			inspector->add(MAKE(ofxCvGui::Widgets::LiveValue<string>, "Window status", [this]() {
				if (!this->window) {
					return string("closed");
				}
				else {
					return string("open");
				}
			}));

			inspector->add(MAKE(ofxCvGui::Widgets::LiveValue<ofVec2f>, "Projector Size", [this]() {
				return ofVec2f(this->getProjectorSize().getWidth(), this->getProjectorSize().getHeight());
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
		}

		//----------
		void ProjectorOutput::update() {
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

			if (this->window) {
				auto mainWindow = glfwGetCurrentContext();
				glfwMakeContextCurrent(this->window);

				ofClear(0, 0);

				const auto viewport = this->getSplitSelectionRect();
				GLint viewportCached[4];
				glGetIntegerv(GL_VIEWPORT, viewportCached);
				glViewport(viewport.x, viewport.y, viewport.width, viewport.height); //<--needs checking for vertical spreads

				//set the drawing matrices to normalised coordinates
				ofSetMatrixMode(OF_MATRIX_PROJECTION);
				ofLoadIdentityMatrix();
				ofSetMatrixMode(OF_MATRIX_MODELVIEW);
				ofLoadIdentityMatrix();
				this->fbo.draw(-1, -1, 2, 2);

				glViewport(viewportCached[0], viewportCached[1], viewportCached[2], viewportCached[3]);

				glfwSwapBuffers(this->window);
				glFlush();
				glfwMakeContextCurrent(mainWindow);

				this->fbo.bind();
				ofClear(0, 255);
				this->fbo.unbind();
			}
		}

		//----------
		ofFbo & ProjectorOutput::getFbo() {
			return this->fbo;
		}

		//----------
		GLFWwindow * ProjectorOutput::getWindow() const {
			return this->window;
		}

		//----------
		ofRectangle ProjectorOutput::getProjectorSize() const {
			return ofRectangle(0.0f, 0.0f, this->width, this->height);
		}

		//----------
		float ProjectorOutput::getWidth() const {
			return this->width;
		}

		//----------
		float ProjectorOutput::getHeight() const {
			return this->height;
		}

		//----------
		ofRectangle ProjectorOutput::getSplitSelectionRect() const {
			float x = (int) this->splitUseIndex % (int) this->splitHorizontal * this->getWidth();
			float y = (int) this->splitUseIndex / (int) this->splitHorizontal * this->getHeight();
			return ofRectangle(x, y, this->getWidth(), this->getHeight());
		}

		//----------
		void ProjectorOutput::applyNormalisedSplitViewTransform() const {
			ofVec2f tanslation = ofVec2f(-1, -1) + 1 / ofVec2f(this->splitHorizontal, this->splitVertical);
			tanslation += ofVec2f(2.0f / this->splitHorizontal, 2.0f / this->splitVertical) *
				ofVec2f((int) this->splitUseIndex % (int) this->splitHorizontal, (int) this->splitUseIndex / (int) this->splitHorizontal);
			ofTranslate(tanslation);
			ofScale(1.0f / this->splitHorizontal, 1.0f / this->splitVertical, 1.0f);
		}

		//----------
		void ProjectorOutput::setWindowOpen(bool windowOpen) {
			if (windowOpen) {
				this->createWindow();
			}
			else {
				this->destroyWindow();
			}
		}

		//----------
		bool ProjectorOutput::isWindowOpen() const {
			return this->window;
		}

		//----------
		void ProjectorOutput::createWindow() {
			this->destroyWindow();

			this->refreshMonitors();

			if (this->monitorSelection < this->monitorCount && this->monitorSelection >= 0) {
				auto monitor = this->monitors[this->monitorSelection];
				int x, y;

				glfwGetMonitorPos(monitor, &x, &y);

				this->videoMode = glfwGetVideoMode(monitor);
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
		void ProjectorOutput::destroyWindow() {
			if (this->window) {
				glfwDestroyWindow(this->window);
				this->window = nullptr;
			}
		}

		//----------
		void ProjectorOutput::calculateSplit() {
			if (!this->window) {
				if (this->monitors && this->monitorSelection < this->monitorCount) {
					glfwGetVideoMode(this->monitors[this->monitorSelection]);
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
		void ProjectorOutput::callbackChangeSplit(float &) {
			this->calculateSplit();
		}
	}
}
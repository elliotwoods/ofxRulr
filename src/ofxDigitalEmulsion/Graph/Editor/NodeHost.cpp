#include "NodeHost.h"
#include "ofxAssets.h"
#include "ofxCvGui.h"

using namespace ofxAssets;

namespace ofxDigitalEmulsion {
	namespace Graph {
		namespace Editor {
			//----------
			NodeHost::NodeHost(shared_ptr<Node> node) {
				this->node = node;
				this->nodeView = node->getView();
				this->nodeView->setCaption(this->node->getName());

				this->elements = make_shared<ofxCvGui::ElementGroup>();
				auto resizeHandle = make_shared<ofxCvGui::Utils::Button>();
				resizeHandle->onDrawUp += [](ofxCvGui::DrawArguments & args) {
					image("ofxDigitalEmulsion::resizeHandle").draw(args.localBounds);
				};
				resizeHandle->onDrawDown += [](ofxCvGui::DrawArguments & args) {
					image("ofxDigitalEmulsion::resizeHandle").draw(args.localBounds);
				};
				resizeHandle->onMouse += [this, resizeHandle](ofxCvGui::MouseArguments & args) {
					if (args.isDragging(resizeHandle.get())) {
						auto newBounds = this->getBounds();
						newBounds.width += args.movement.x;
						newBounds.height += args.movement.y;
						this->setBounds(newBounds);
					}
				};
				this->elements->add(resizeHandle);

				auto outputPinView = make_shared<PinView>();
				outputPinView->setup(*this->getNodeInstance());
				this->elements->add(outputPinView);

				this->elements->onBoundsChange += [resizeHandle, outputPinView, this](ofxCvGui::BoundsChangeArguments & args) {
					auto & resizeImage = image("ofxDigitalEmulsion::resizeHandle");
					resizeHandle->setBounds(ofRectangle(args.bounds.getWidth() - resizeImage.getWidth(), args.bounds.getHeight() - resizeImage.getHeight(), resizeImage.getWidth(), resizeImage.getHeight()));
					const auto iconSize = 48;
					outputPinView->setBounds(ofRectangle(this->getOutputPinPosition() - ofVec2f(iconSize + 32, iconSize / 2), iconSize, iconSize));
				};

				this->inputPins = make_shared<ofxCvGui::ElementGroup>();
				for (auto inputPin : node->getInputPins()) {
					this->inputPins->add(inputPin);
					inputPin->onBeginMakeConnection += [this, inputPin](ofEventArgs &) {
						this->onBeginMakeConnection(inputPin);
					};
					inputPin->onReleaseMakeConnection += [this, inputPin](ofxCvGui::MouseArguments & args) {
						this->onReleaseMakeConnection(args);
					};
				};
				this->inputPins->onBoundsChange += [this](ofxCvGui::BoundsChangeArguments & args) {
					this->inputPins->layoutGridVertical();
				};
				this->inputPins->onDraw.addListener([this](ofxCvGui::DrawArguments & args) {
					ofPushStyle();
					ofSetLineWidth(1.0f);
					ofSetColor(50);
					bool first = true;
					for (auto pin : this->inputPins->getElements()) {
						if (first) {
							first = false;
						}
						else {
							ofLine(pin->getBounds().getTopLeft(), pin->getBounds().getTopRight());
						}
					}
					ofPopStyle();
				}, -1, this);

				this->onUpdate += [this](ofxCvGui::UpdateArguments & args) {
					this->getNodeInstance()->update();
				};

				this->onDraw += [this](ofxCvGui::DrawArguments & args) {
					ofPushStyle();

					//shadow for node
					ofFill();
					ofSetColor(0, 100);
					ofPushMatrix();
					ofTranslate(5, 5);
					ofRect(this->getLocalBounds());
					ofPopMatrix();

					//background for node
					ofSetColor(80);
					ofRect(this->getLocalBounds());

					//background for nodeView
					ofSetColor(30);
					ofRect(this->nodeView->getBounds());
					ofPopStyle();

					//output pin
					ofPushStyle();
					ofSetLineWidth(6.0f);
					ofLine(this->getOutputPinPosition(), this->getOutputPinPosition() - ofVec2f(10.0f, 0.0f));
					ofPopStyle();
				};

				this->onBoundsChange += [this, resizeHandle](ofxCvGui::BoundsChangeArguments & args) {
					const int minWidth = 200 + 200;
					const int minHeight = 150;

					if (args.bounds.width < minWidth || args.bounds.height < minHeight) {
						auto fixedBounds = args.bounds;
						fixedBounds.width = MAX(args.bounds.width, minWidth);
						fixedBounds.height = MAX(args.bounds.height, minHeight);
						this->setBounds(fixedBounds);
						return;
					}

					auto viewBounds = args.localBounds;
					viewBounds.x = 85;
					viewBounds.width -= 185;
					viewBounds.y = 1;
					viewBounds.height -= 2;
					if (this->nodeView) {
						this->nodeView->setBounds(viewBounds);
					}

					this->inputPins->setBounds(ofRectangle(0, 0, viewBounds.x, this->getHeight()));

					this->outputPinPosition = ofVec2f(this->getWidth(), this->getHeight() / 2.0f);
				};

				this->onMouse += [this](ofxCvGui::MouseArguments & args) {
					if (args.isDragging(this)) {
						auto newBounds = this->getBounds();
						newBounds.x += args.movement.x;
						newBounds.y += args.movement.y;
						if (newBounds.x < 0) {
							newBounds.x = 0;
						}
						if (newBounds.y < 0) {
							newBounds.y = 0;
						}
						this->setBounds(newBounds);
					}
					
					//if no children took the mouse press, then we'll have it to block it going down
					args.takeMousePress(this);

					if (args.action == ofxCvGui::MouseArguments::Pressed) {
						// mouse went down somewhere inside this element (regardless of where it's taken)
						ofxCvGui::inspect(this->getNodeInstance());
					}
				};

				if (this->nodeView) {
					this->nodeView->addListenersToParent(this);
				}
				this->elements->addListenersToParent(this, true);
				this->inputPins->addListenersToParent(this);

				this->setBounds(ofRectangle(200, 200, 200, 200));
			}

			//----------
			shared_ptr<Node> NodeHost::getNodeInstance() {
				return this->node;
			}

			//----------
			ofVec2f NodeHost::getInputPinPosition(shared_ptr<BasePin> pin) const {
				for (auto inputPin : this->inputPins->getElements()) {
					if (inputPin == pin) {
						return pin->getPinHeadPosition() + pin->getBounds().getTopLeft() + this->inputPins->getBounds().getTopLeft() + this->getBounds().getTopLeft();
					}
				}
				throw(ofxDigitalEmulsion::Utils::Exception("NodeHost::getInputPinPosition can't find input pin" + ofToString((int) pin.get())));
			}

			//----------
			ofVec2f NodeHost::getOutputPinPositionGlobal() const {
				return this->getOutputPinPosition() + this->getBounds().getTopLeft();
			}

			//----------
			ofVec2f NodeHost::getOutputPinPosition() const {
				return this->outputPinPosition;
			}
		}
	}
}
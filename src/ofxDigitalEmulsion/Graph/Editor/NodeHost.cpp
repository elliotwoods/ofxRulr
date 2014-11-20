#include "NodeHost.h"
#include "ofxAssets.h"

using namespace ofxAssets;

namespace ofxDigitalEmulsion {
	namespace Graph {
		namespace Editor {
			//----------
			NodeHost::NodeHost(shared_ptr<Node> node) {
				this->node = node;
				this->nodeView = node->getView();
				this->nodeView->setCaption(this->node->getName());
				/*
			Todo :
				* inspect the node when click on the view
				* drag to move / resize
				* delete button
				*/

				this->elements = make_shared<ofxCvGui::ElementGroup>();

				auto deleteButton = make_shared<ofxCvGui::Utils::Button>();
				deleteButton->onDrawUp += [](ofxCvGui::DrawArguments & args) {
					image("ofxDigitalEmulsion::cross").draw(args.localBounds);
				};
				deleteButton->onDrawDown += [](ofxCvGui::DrawArguments & args) {
					ofPushStyle();
					ofSetColor(100);
					image("ofxDigitalEmulsion::cross").draw(args.localBounds);
					ofPopStyle();
				};
				this->elements->add(deleteButton);
				
				auto resizeHandle = make_shared<ofxCvGui::Utils::Button>();
				resizeHandle->onDrawUp += [](ofxCvGui::DrawArguments & args) {
					image("ofxDigitalEmulsion::scaleAlongLine").draw(args.localBounds);
				};
				resizeHandle->onDrawDown += [](ofxCvGui::DrawArguments & args) {
					ofPushStyle();
					ofSetColor(100);
					image("ofxDigitalEmulsion::scaleAlongLine").draw(args.localBounds);
					ofPopStyle();
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

				auto pinView = make_shared<PinView>(node->getTypeName());
				this->elements->add(pinView);

				this->elements->onBoundsChange += [deleteButton, resizeHandle, pinView, this](ofxCvGui::BoundsChangeArguments & args) {
					deleteButton->setBounds(ofRectangle(args.bounds.getWidth() - 32 - 4, 4, 32, 32));
					resizeHandle->setBounds(ofRectangle(args.bounds.getWidth() - 32 - 4, args.bounds.getHeight() - 32 - 4, 32, 32));
					pinView->setBounds(ofRectangle(this->getOutputPinPosition() - ofVec2f(16 + 64, 32), 64, 64));
				};

				this->inputPins = make_shared<ofxCvGui::ElementGroup>();
				for (auto inputPin : node->getInputPins()) {
					this->inputPins->add(inputPin);
					inputPin->onBeginMakeConnection += [this, inputPin](ofEventArgs &) {
						this->onBeginMakeConnection(inputPin);
					};
					inputPin->onReleaseMakeConnection += [this, inputPin](ofEventArgs &) {
						this->onReleaseMakeConnection(inputPin);
					};
				};
				this->inputPins->onBoundsChange += [this](ofxCvGui::BoundsChangeArguments & args) {
					this->inputPins->layoutGridVertical(20.0f);
				};

				this->onDraw += [this](ofxCvGui::DrawArguments & args) {
					ofPushStyle();

					//shadow for node
					ofFill();
					ofSetColor(0, 100);
					ofPushMatrix();
					ofTranslate(5, 5);
					ofRectRounded(this->getLocalBounds(), 16.0f);
					ofPopMatrix();

					//background for node
					ofSetColor(100);
					ofRectRounded(this->getLocalBounds(), 16.0f);

					//background for nodeView
					ofSetColor(30);
					ofRect(this->nodeView->getBounds());
					ofPopStyle();

					//output pin
					ofPushStyle();
					ofFill();
					ofCircle(this->getOutputPinPosition(), 5.0f);
					ofPopStyle();
				};

				this->onBoundsChange += [this, deleteButton, resizeHandle](ofxCvGui::BoundsChangeArguments & args) {
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
					viewBounds.width -= 195;
					viewBounds.y = 2;
					viewBounds.height -= 4;
					this->nodeView->setBounds(viewBounds);
					this->inputPins->setBounds(ofRectangle(2, 10, viewBounds.x - 4, this->getHeight() - 20));

					this->outputPinPosition = ofVec2f(this->getWidth() - 12, this->getHeight() / 2.0f);
				};

				this->onMouse += [this](ofxCvGui::MouseArguments & args) {
					//if didn't click anything inside, we'll have the click ourselves
					args.takeMousePress(this);

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
				};

				this->nodeView->addListenersToParent(this);
				this->elements->addListenersToParent(this, true);
				this->inputPins->addListenersToParent(this);

				this->setBounds(ofRectangle(600, 300, 200, 200));
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
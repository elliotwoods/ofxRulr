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
					if (args.action == ofxCvGui::MouseArguments::Action::Dragged) {
						auto newBounds = this->getBounds();
						newBounds.width += args.movement.x;
						newBounds.height += args.movement.y;
						this->setBounds(newBounds);
					}
				};
				this->elements->add(resizeHandle);

				this->nodeView->addListenersToParent(this);
				this->elements->addListenersToParent(this, true);

				this->onDraw.addListener([this](ofxCvGui::DrawArguments & args) {
					ofPushStyle();

					//background for node
					ofFill();
					ofSetColor(100);
					ofRectRounded(this->getLocalBounds(), 10.0f);

					//background for nodeView
					ofSetColor(30);
					ofRect(this->nodeView->getBounds());
					ofPopStyle();
				}, -1, this);

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
					viewBounds.x += 100;
					viewBounds.width -= 200;
					viewBounds.y += 2;
					viewBounds.height -= 4;
					this->nodeView->setBounds(viewBounds);

					deleteButton->setBounds(ofRectangle(this->getWidth() - 32 - 4, 4, 32, 32));
					resizeHandle->setBounds(ofRectangle(this->getWidth() - 32 - 4, this->getHeight() - 32 - 4, 32, 32));
					
					auto node = this->getNodeInstance();
					auto inputPins = node->getInputPins();
					int pinIndex = 0;
					for (auto inputPin : inputPins) {
						inputPin->setBounds(ofRectangle(0, 20 + 20 * pinIndex, 100, 20));
					}
				};

				this->onMouse += [this](ofxCvGui::MouseArguments & args) {
					//if didn't click anything inside, we'll have the click ourselves
					args.takeMousePress(this);

					if (args.action == ofxCvGui::MouseArguments::Action::Dragged && args.getOwner() == this) {
						auto newBounds = this->getBounds();
						newBounds.x += args.movement.x;
						newBounds.y += args.movement.y;
						this->setBounds(newBounds);
					}
				};

				this->setBounds(ofRectangle(600, 300, 200, 200));
			}

			//----------
			shared_ptr<Node> NodeHost::getNodeInstance() {
				return this->node;
			}
		}
	}
}
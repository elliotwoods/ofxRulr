#include "Pin.h"
#include "../../../addons/ofxCvGui/src/ofxCvGui/Utils/Utils.h"

namespace ofxDigitalEmulsion {
	namespace Graph {
		//----------
		BasePin::BasePin(string name) : name(name) {
			this->pinView = make_shared<Editor::PinView>();

			this->onDraw += [this](ofxCvGui::DrawArguments & args) {
				//hover mouse
				if (this->isMouseOver() && this->getMouseState() == LocalMouseState::Waiting) {
					ofPushStyle();
					ofSetColor(100);
					ofRect(args.localBounds);
					ofPopStyle();
				}

				ofPushStyle();
				ofSetLineWidth(6.0f);
				if (this->isConnected()) {
					ofSetColor(100, 200, 100);
				}
				ofLine(this->getPinHeadPosition(), this->getPinHeadPosition() + ofVec2f(10.0f, 0.0f));
				ofPopStyle();
			};

			this->onMouse += [this](ofxCvGui::MouseArguments & args) {
				if (args.takeMousePress(this)) {
					if (args.button == 0) {
						ofEventArgs dummyArgs;
						this->onBeginMakeConnection(dummyArgs);
						this->globalElementPosition = args.global - args.local;
					}
					else if (args.button == 2) {
						this->resetConnection();
					}
					
				}
				else if (args.action == ofxCvGui::MouseArguments::Action::Released) {
					this->onReleaseMakeConnection(args);
				}
			};

			this->onBoundsChange += [this](ofxCvGui::BoundsChangeArguments & args) {
				auto ourBounds = this->getLocalBounds();
				this->pinHeadPosition = ourBounds.getTopLeft() + ofVec2f(0.0f, ourBounds.getHeight() / 2.0f);
				this->pinView->setBounds(ofRectangle(this->getPinHeadPosition() + ofVec2f(32, -16), 32, 32));
			};

			this->setBounds(ofRectangle(0, 0, 100, 20));

			this->pinView->addListenersToParent(this);
		}
		
		//----------
		string BasePin::getName() const {
			return this->name;
		}

		//----------
		ofVec2f BasePin::getPinHeadPosition() const {
			return this->pinHeadPosition;
		}
	}
}
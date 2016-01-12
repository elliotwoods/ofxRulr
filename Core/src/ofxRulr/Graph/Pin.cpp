#include "pch_RulrCore.h"
#include "Pin.h"

#include "../../../addons/ofxCvGui/src/ofxCvGui/Utils/Utils.h"

namespace ofxRulr {
	namespace Graph {
		//----------
		AbstractPin::AbstractPin(string name) : name(name) {
			this->pinView = make_shared<Editor::PinView>();

			this->onDraw += [this](ofxCvGui::DrawArguments & args) {
				//hover mouse = change background
				if (this->isMouseOver() && this->getMouseState() == LocalMouseState::Waiting) {
					ofPushStyle();
					{
						ofSetColor(100);
						ofDrawRectangle(args.localBounds);
					}
					ofPopStyle();
				}

				//line to indicate connection status
				ofPushStyle();
				{
					ofSetLineWidth(0.0f);
					ofFill();
					if (this->isConnected()) {
						ofSetColor(100, 200, 100);
					}

					ofPushMatrix();
					{
						ofTranslate(this->getPinHeadPosition());
						ofDrawRectangle(0, -3.0f, 10.0f, 6.0f);
					}
					ofPopMatrix();
				}
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
				if (args.action == ofxCvGui::MouseArguments::Action::Released && args.getOwner() == this) {
					//mouse drag is released, we took the mouse, and it could have been released outside of this element
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
		string AbstractPin::getName() const {
			return this->name;
		}

		//----------
		ofVec2f AbstractPin::getPinHeadPosition() const {
			return this->pinHeadPosition;
		}
	}
}
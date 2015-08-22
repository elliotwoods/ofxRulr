#include "Pin.h"

#include "ofxRulr/Graph/Editor/Patch.h"
#include "ofxCvGui/Utils/Utils.h"

#include "ofxCvGui/Widgets/Toggle.h"

using namespace ofxCvGui;

namespace ofxRulr {
	namespace Graph {
#pragma mark AbstractPin::Widget
		//----------
		AbstractPin::Widget::Widget(AbstractPin & hostPin) :
			hostPin(hostPin) {
			this->elements = make_shared<ElementGroup>();
			this->elements->addListenersToParent(this, true);

			//--
			//expose pin toggle
			//--
			//
			auto exposePinButton = Widgets::Toggle::make("Expose", [this]() {
				return this->hostPin.getIsExposedThroughParentPatch();
			}, [this](bool exposePin) {
				this->hostPin.setIsExposedThroughParentPatch(exposePin);
			});
			//
			//only show the toggle if we have a parent patch
			auto exposePinButtonWeak = weak_ptr<Widgets::Toggle>(exposePinButton);
			this->onUpdate += [this, exposePinButtonWeak](UpdateArguments &) {
				auto exposePinButton = exposePinButtonWeak.lock();
				if (exposePinButton) {
					if (this->hostPin.getParentPatch()) {
						exposePinButton->setEnabled(true);
					}
					else {
						exposePinButton->setEnabled(false);
					}
				}
			};
			this->elements->add(exposePinButton);
			//
			//--
			this->onDraw += [this](DrawArguments & args) {
				auto & font = ofxAssets::font(ofxCvGui::defaultTypeface, 12);
				font.drawString(this->hostPin.getName(), 50, 20);
				font.drawString(this->hostPin.getTypeName(), 50, 40);

				ofPushStyle();
				{
					if (this->hostPin.isConnected()) {
						ofFill();
						ofSetLineWidth(0.0f);
						ofSetColor(100, 200, 100);
						ofCircle(20, 20, 9);
					}
					ofSetLineWidth(1.0f);
					ofNoFill();
					ofSetColor(255);
					ofCircle(20, 20, 9);
				}
				ofPopStyle();
			};

			this->onBoundsChange += [this, exposePinButtonWeak](BoundsChangeArguments & args) {
				auto exposePinButton = exposePinButtonWeak.lock();
				if (exposePinButton) {
					exposePinButton->setBounds(ofRectangle(args.localBounds.width - 80.0f, 0, 80.0f, args.localBounds.height));
				}
			};

			this->setBounds(ofRectangle(0, 0, 100, 60.0f));
		}
#pragma mark AbstractPin
		//----------
		AbstractPin::AbstractPin(string name) : name(name) {
			this->pinView = make_shared<Editor::PinView>();

			this->onDraw += [this](ofxCvGui::DrawArguments & args) {
				//hover mouse = change background
				if (this->isMouseOver() && this->getMouseState() == LocalMouseState::Waiting) {
					ofPushStyle();
					{
						ofSetColor(100);
						ofRect(args.localBounds);
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

			this->isExposedThroughParentPatch = false;
			this->parentNode = nullptr;
			this->parentPatch = nullptr;
		}
		
		//----------
		void AbstractPin::setParentNode(Nodes::Base * parentNode) {
			this->parentNode = parentNode;
		}

		//----------
		void AbstractPin::setParentPatch(Graph::Editor::Patch * parentPatch) {
			this->parentPatch = parentPatch;
		}

		//----------
		Graph::Editor::Patch * AbstractPin::getParentPatch() const {
			return this->parentPatch;
		}

		//----------
		string AbstractPin::getName() const {
			return this->name;
		}

		//----------
		ofVec2f AbstractPin::getPinHeadPosition() const {
			return this->pinHeadPosition;
		}

		//----------
		shared_ptr<AbstractPin::Widget> AbstractPin::getWidget() {
			if (!this->widget) {
				this->widget = make_shared<Widget>(*this);
			}
			return this->widget;
		}

		//----------
		void AbstractPin::setIsExposedThroughParentPatch(bool pinIsExposed) {
			if (pinIsExposed != this->isExposedThroughParentPatch) {
				auto parentPatch = this->parentPatch;
				if (parentPatch) {
					if (pinIsExposed) {
						this->isExposedThroughParentPatch = true;
						parentPatch->exposePin(shared_from_this(), this->parentNode);
						parentNode->onExposedPinsChanged.notifyListeners();
					}
					else {
						this->resetConnection();
						this->isExposedThroughParentPatch = false;
						parentPatch->unexposePin(shared_from_this());
						parentNode->onExposedPinsChanged.notifyListeners();
					}
				}
			}
		}

		//----------
		bool AbstractPin::getIsExposedThroughParentPatch() const {
			return isExposedThroughParentPatch;
		}

		//----------
		bool AbstractPin::isVisibleInPatch(Graph::Editor::Patch * patch) const {
			if (this->getIsExposedThroughParentPatch() && patch == this->parentPatch) {
				//asking if visible in our host patch, but it's exposed through to that patch's parent
				return false;
			}
			else {
				return true;
			}
		}
	}
}
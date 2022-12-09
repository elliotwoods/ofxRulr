#include "pch_Plugin_Scrap.h"
#include "Moon.h"

namespace ofxRulr {
	namespace Nodes {
		namespace AnotherMoon {
			//------------
			Moon::Moon()
			{
				RULR_NODE_INIT_LISTENER;
			}

			//------------
			string
				Moon::getTypeName() const
			{
				return "AnotherMoon::Moon";
			}

			//------------
			void 
				Moon::init()
			{
				RULR_NODE_UPDATE_LISTENER;
				RULR_RIGIDBODY_DRAW_OBJECT_LISTENER;
				RULR_NODE_INSPECTOR_LISTENER;

				this->manageParameters(this->parameters);

				// Trigger events on moon change
				this->parameters.diameter.addListener(this, &Moon::callbackDiameter, 0);
				this->onTransformChange += [this]() {
					this->onMoonChange.notifyListeners();
					};

				// Set different bounds (for easier editing)
				{
					for (int i = 0; i < 3; i++) {
						RigidBody::translation[i].setMin(-50);
						RigidBody::translation[i].setMax(50);
					}
				}
			}

			//------------
			void
				Moon::update()
			{
				if (this->oscReceiver) {
					if (this->parameters.oscReceiver.port.get() != this->oscReceiver->getPort()) {
						this->oscReceiver.reset();
					}

					if (!this->parameters.oscReceiver.enabled) {
						this->oscReceiver.reset();
					}
				}

				if (!this->oscReceiver && this->parameters.oscReceiver.enabled) {
					this->oscReceiver = make_unique<ofxOscReceiver>();
					this->oscReceiver->setup(this->parameters.oscReceiver.port.get());
				}
				
				if (this->oscReceiver) {
					ofxOscMessage message;
					while (this->oscReceiver->getNextMessage(message)) {
						this->oscFrameNew.notifyFrameNew = true;

						if (message.getAddress() == "/position" && message.getNumArgs() >= 3) {
							glm::vec3 position{
								message.getArgAsFloat(0)
								, message.getArgAsFloat(1)
								, message.getArgAsFloat(2)
							};
							this->setPosition(position);
						}

						if (message.getAddress() == "/radius" && message.getNumArgs() >= 1) {
							this->parameters.diameter.set(message.getArgAsFloat(0) * 2.0f);
						}

						if (message.getAddress() == "/diameter" && message.getNumArgs() >= 1) {
							this->parameters.diameter.set(message.getArgAsFloat(0));
						}

						if (message.getAddress() == "/moon" && message.getNumArgs() >= 4) {
							glm::vec3 position{
								message.getArgAsFloat(0)
								, message.getArgAsFloat(1)
								, message.getArgAsFloat(2)
							};
							this->setPosition(position);

							this->parameters.diameter.set(message.getArgAsFloat(3));
						}
					}
				}

				{
					this->oscFrameNew.isFrameNew = this->oscFrameNew.notifyFrameNew;
					this->oscFrameNew.notifyFrameNew = false;
				}
			}

			//------------
			void
				Moon::populateInspector(ofxCvGui::InspectArguments& args)
			{
				auto inspector = args.inspector;
				inspector->addHeartbeat("OSC rx", [this]() {
					return this->oscFrameNew.isFrameNew;
					});
			}

			//------------
			void
				Moon::drawObject()
			{
				ofPushStyle();
				{
					if (ofxRulr::isActive(this, this->parameters.draw.filled.get())) {
						ofFill();
					}
					else {
						ofNoFill();
					}
					ofSetSphereResolution(this->parameters.draw.resolution.get());
					ofDrawSphere(this->getRadius());
				}
				ofPopStyle();
			}

			//------------
			float
				Moon::getRadius() const
			{
				return this->parameters.diameter.get() / 2.0f;
			}

			//------------
			void
				Moon::setDiameter(float value)
			{
				this->parameters.diameter.set(value);
			}

			//------------
			void
				Moon::callbackDiameter(float&)
			{
				this->onMoonChange.notifyListeners();
			}
		}
	}
}

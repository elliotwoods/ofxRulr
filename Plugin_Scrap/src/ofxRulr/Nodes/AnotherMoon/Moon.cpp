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

			}

			//------------
			void
				Moon::populateInspector(ofxCvGui::InspectArguments& args)
			{

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

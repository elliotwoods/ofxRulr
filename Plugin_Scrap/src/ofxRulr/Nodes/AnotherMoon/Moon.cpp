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
				RULR_RIGIDBODY_DRAW_OBJECT_LISTENER;

				this->manageParameters(this->parameters);
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
		}
	}
}

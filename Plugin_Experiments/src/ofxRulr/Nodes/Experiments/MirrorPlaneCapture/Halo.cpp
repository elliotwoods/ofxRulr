#include "pch_Plugin_Experiments.h"

namespace ofxRulr {
	namespace Nodes {
		namespace Experiments {
			namespace MirrorPlaneCapture {				
				//---------
				Halo::Halo() {
					RULR_NODE_INIT_LISTENER;
				}

				//---------
				string Halo::getTypeName() const {
					return "Halo::Halo";
				}

				//---------
				void Halo::init() {
					RULR_RIGIDBODY_DRAW_OBJECT_LISTENER;

					this->manageParameters(this->parameters);
				}

				//---------
				void Halo::drawObject() {
					ofPushMatrix();
					{
						ofRotateDeg(90, {1.0f, 0.0f, 0.0f});
						ofDrawCircle(0.0f, 0.0f, this->parameters.diameter.get() / 2.0f);
					}
					ofPopMatrix();
				}
			}
		}
	}
}
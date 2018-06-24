#pragma once

#include "ofxRulr.h"

namespace ofxRulr {
	namespace Nodes {
		namespace Experiments {
			namespace SolvePnP {
				class TestObject : public Nodes::Item::RigidBody {
				public:
					TestObject();
					string getTypeName() const override;
					void init();
					void drawObject();
					vector<ofVec3f> getObjectPoints();
					vector<ofVec3f> getWorldPoints();
				protected:
					struct : ofParameterGroup {
						ofParameter<float> diameter{ "Diameter [m]", 0.1, 0.01, 1.0 };
						PARAM_DECLARE("TestObject", diameter);
					} parameters;

					ofxRay::Plane planeObject;
				};
			}
		}
	}
}
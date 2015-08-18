#pragma once

#include "Fixture.h"
#include "ofxRulr/Nodes/Item/RigidBody.h"

namespace ofxRulr {
	namespace Nodes {
		namespace DMX {
			class MovingHead : public Item::RigidBody, public Fixture {
			public:
				MovingHead();
				void init();
				string getTypeName() const override;

				void serialize(Json::Value &);
				void deserialize(const Json::Value &);

				void lookAt(const ofVec3f & worldSpacePoint);

				float getPan() const;
				float getTilt() const;
			protected:
				void populateInspector(ofxCvGui::ElementGroupPtr);

				ofParameter<float> pan;
				ofParameter<float> tilt;
				ofParameter<float> brightness;
				ofParameter<bool> power;
				ofParameter<string> powerCircuit;
				ofParameter<int> pauseBetweenPowerUps;
			};
		}
	}
}
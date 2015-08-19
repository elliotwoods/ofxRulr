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

				ofVec2f getPanTilt() const;
				ofVec2f getPanTiltForTarget(const ofVec3f & worldSpacePoint, bool closestToCurrent) const;
				void lookAt(const ofVec3f & worldSpacePoint); /// warning : throws exception if impossible
				void setPanTilt(const ofVec2f & panTilt);
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
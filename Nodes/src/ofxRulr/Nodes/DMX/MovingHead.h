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
				void update();

				void serialize(Json::Value &);
				void deserialize(const Json::Value &);

				float getMaxTilt() const;
				ofVec2f getPanTilt() const;
				ofVec2f getPanTiltForTarget(const ofVec3f & worldSpacePoint, bool navigationEnabled) const; ///navigationEnabled uses closest path, also throws on impossible target
				static ofVec2f getPanTiltForTargetInObjectSpace(const ofVec3f & objectSpacePoint, float tiltOffset = 0.0f);
				void lookAt(const ofVec3f & worldSpacePoint); /// warning : throws exception if impossible
				void setPanTilt(const ofVec2f & panTilt);

				void setBrightness(float);
				void setIris(float);
				void setPower(bool);
				void setHome();

				void setTiltOffset(float);
			protected:
				void populateInspector(ofxCvGui::ElementGroupPtr);

				ofParameter<float> pan;
				ofParameter<float> tilt;
				ofParameter<float> brightness;
				ofParameter<float> iris;
				ofParameter<bool> power;
				ofParameter<string> powerCircuit;
				ofParameter<int> pauseBetweenPowerUps;
				ofParameter<float> tiltOffset;

				//you should generate your lamp signal based on 'power state signal' rather than on power parameter
				bool powerStateSignal;
				ofxLiquidEvent<void> onPowerOn;
				ofxLiquidEvent<void> onPowerOff;
			};
		}
	}
}
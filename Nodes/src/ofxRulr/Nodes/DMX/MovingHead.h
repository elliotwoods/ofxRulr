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
				void copyFrom(shared_ptr<MovingHead>);
			protected:
				void populateInspector(ofxCvGui::InspectArguments &);

				struct : ofParameterGroup {
					ofParameter<float> pan{ "Pan", 0.0f, -180.0f, 180.0f };
					ofParameter<float> tilt{ "Tilt", 0.0f, -100.0f, 100.0f };
					ofParameter<float> brightness{ "Brightness", 0.0f, 0.0f, 1.0f };
					ofParameter<float> iris{ "Iris", 1.0f, 0.0f, 1.0f };
					ofParameter<bool> power{ "Power", false };
					ofParameter<string> powerCircuit{ "Power circuit (mutex)", "main ring" };
					ofParameter<int> pauseBetweenPowerUps{ "Pause between power ups [s]", 2 };
					ofParameter<float> tiltOffset{ "Tilt offset", 0.0f, -90.0f, 90.0f };

					PARAM_DECLARE("Moving head", pan, tilt, brightness, iris, power, powerCircuit, pauseBetweenPowerUps, tiltOffset);
				} parameters;

				//you should generate your lamp signal based on 'power state signal' rather than on power parameter
				bool powerStateSignal;
				ofxLiquidEvent<void> onPowerOn;
				ofxLiquidEvent<void> onPowerOff;
			};
		}
	}
}
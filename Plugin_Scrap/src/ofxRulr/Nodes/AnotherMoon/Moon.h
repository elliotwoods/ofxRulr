#pragma once

#include "ofxRulr.h"
#include "ofxRulr/Nodes/Item/RigidBody.h"
#include "ofxOsc.h"

namespace ofxRulr {
	namespace Nodes {
		namespace AnotherMoon {
			class Moon : public Nodes::Item::RigidBody {
			public:
				Moon();
				string getTypeName() const override;

				void init();
				void update();

				void populateInspector(ofxCvGui::InspectArguments&);

				void drawObject();

				float getRadius() const;

				ofxLiquidEvent<void> onMoonChange;
			protected:
				struct : ofParameterGroup {
					ofParameter<float> diameter{ "Diameter", 5.0f };

					struct : ofParameterGroup {
						ofParameter<int> resolution{ "Resolution", 5 };
						ofParameter<WhenActive> filled{ "Filled", WhenActive::Selected };
						PARAM_DECLARE("Draw", resolution, filled);
					} draw;

					struct : ofParameterGroup {
						ofParameter<bool> enabled{ "Enabled", false };
						ofParameter<int> port{ "Port", 5000 };
						PARAM_DECLARE("Osc Receiver", enabled, port);
					} oscReceiver;

					PARAM_DECLARE("Moon", diameter, draw, oscReceiver);
				} parameters;

				ofEventListener listenerDiameter;
				unique_ptr<ofxOscReceiver> oscReceiver;

				void callbackDiameter(float&);

				struct {
					bool isFrameNew = false;
					bool notifyFrameNew = false;
				} oscFrameNew;
			};
		}
	}
}
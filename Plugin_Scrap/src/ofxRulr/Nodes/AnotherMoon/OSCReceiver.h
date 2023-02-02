#pragma once

#include "ofxRulr.h"
#include "ofxOsc.h"

namespace ofxRulr {
	namespace Nodes {
		namespace AnotherMoon {
			class OSCReceiver : public Nodes::Base {
			public:
				OSCReceiver();
				string getTypeName() const override;

				void init();
				void update();

				void populateInspector(ofxCvGui::InspectArguments&);
			protected:
				struct : ofParameterGroup {
					ofParameter<bool> enabled{ "Enabled", false };
					ofParameter<int> port{ "Port", 5000 };
					ofParameter<float> maxRxTimePerFrame{ "Max Rx time per frame [s]", 0.1 };
					PARAM_DECLARE("Osc Receiver", enabled, port, maxRxTimePerFrame);
				} parameters;

				unique_ptr<ofxOscReceiver> oscReceiver;

				struct {
					bool isFrameNew = false;
					bool notifyFrameNew = false;
				} oscFrameNew;
			};
		}
	}
}
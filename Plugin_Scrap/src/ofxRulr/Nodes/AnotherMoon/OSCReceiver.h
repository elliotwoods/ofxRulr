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
					PARAM_DECLARE("Osc Receiver", enabled, port);
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
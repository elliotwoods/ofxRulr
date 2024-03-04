#pragma once

#include "ofxRulr/Nodes/Base.h"
#include "ofxOsc.h"
#include "ofxRulr/Data/Dosirak/Curve.h"

namespace ofxRulr {
	namespace Nodes {
		namespace Dosirak {
			class OSCReceive : public Nodes::Base {
			public:
				OSCReceive();
				string getTypeName() const override;
				void init();
				void update();
				void drawWorldStage();

				ofxCvGui::PanelPtr getPanel() override;
				void populateInspector(ofxCvGui::InspectArguments &);
			protected:
				void checkClose();
				void checkOpen();

				struct Parameters: ofParameterGroup {
					ofParameter<bool> enabled{ "Enabled", false };
					ofParameter<int> port{ "Port", 9000 };
					ofParameter<string> address{ "Address", "/curvebundle" };
					PARAM_DECLARE("OSCReceive", enabled, port, address);
				} parameters;

				ofxCvGui::PanelPtr panel;

				unique_ptr<ofxOscReceiver> oscReceiver;

				Data::Dosirak::Curves curves;

				bool hasNewMessage = false;
			};
		}
	}
}
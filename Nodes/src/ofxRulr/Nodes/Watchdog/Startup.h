#pragma once

#include "ofxRulr/Nodes/Base.h"

namespace ofxRulr {
	namespace Nodes {
		namespace Watchdog {
			class Startup : public ofxRulr::Nodes::Base {
			public:
				Startup();
				string getTypeName() const override;
				void init();
				void update();
				void populateInspector(ofxCvGui::InspectArguments &);
			protected:
				struct : ofParameterGroup {
					ofParameter<int> triggerOnFrameNumber{ "Trigger on frame #", 5 };

					struct : ofParameterGroup {
						ofParameter<bool> enabled{ "Enabled", true };
						ofParameter<bool> maximiseInspector{ "Maximise inspector", false };
						PARAM_DECLARE("Inspect node", enabled, maximiseInspector);
					} inspectNode;

					struct : ofParameterGroup {
						ofParameter<bool> enabled{ "Enabled", false };
						ofParameter<ofRectangle> bounds{ "Bounds", {300, 50, 300, 800} };
						PARAM_DECLARE("Reshape window", enabled, bounds);
					} reshapeWindow;

					PARAM_DECLARE("Startup", triggerOnFrameNumber, inspectNode, reshapeWindow);
				} parameters;

				bool forceSimulate = false;
			};
		}
	}
}
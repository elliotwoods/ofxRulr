#pragma once

#include "ofxRulr/Nodes/Base.h"

namespace ofxRulr {
	namespace Nodes {
		namespace Watchdog {
			class Camera : public ofxRulr::Nodes::Base {
			public:
				Camera();
				string getTypeName() const override;
				void init();
				void update();
				void populateInspector(ofxCvGui::InspectArguments &);
			protected:
				struct : ofParameterGroup {
					struct : ofParameterGroup {
						ofParameter<bool> enabled{ "Enabled", true };
						ofParameter<float> timeout{ "Timeout [s]", 10.0f };
						PARAM_DECLARE("Reopen when no new frames", enabled, timeout);
					} reopnWhenNoNewFrames;

					struct : ofParameterGroup {
						ofParameter<bool> enabled{ "Enabled", false };
						ofParameter<float> timeout{ "Timeout [s]", 60.0f };
						PARAM_DECLARE("Reboot when no new frames", enabled, timeout);
					} rebootWhenNoNewFrames;

					PARAM_DECLARE("Camera", reopnWhenNoNewFrames, rebootWhenNoNewFrames);
				} parameters;

				atomic<chrono::high_resolution_clock::time_point> lastCaptureTime;
			};
		}
	}
}
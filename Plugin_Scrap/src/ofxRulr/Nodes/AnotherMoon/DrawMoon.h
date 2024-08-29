#pragma once

#include "ofxRulr.h"

namespace ofxRulr {
	namespace Nodes {
		namespace AnotherMoon {
			class DrawMoon : public Nodes::Base {
			public:
				DrawMoon();
				string getTypeName() const override;

				void init();
				void update();
				void drawWorldStage();
				void populateInspector(ofxCvGui::InspectArguments&);

				void drawLasers(bool throwIfPictureOutsideLimits);
				void searchHeight(float range);
			protected:
				struct : ofParameterGroup {
					ofParameter<WhenActive> live{ "Live", WhenActive::Never };
					ofParameter<WhenActive> onMoonChange { "On Moon change", WhenActive::Never };
					ofParameter<bool> errorIfOutsideRange{ "Error if outside range", true };

					ofParameter<float> resolution{ "Resolution", 256, 256, 1024 };

					struct : ofParameterGroup {
						ofParameter<bool> enabled{ "Enabled", true };
						ofParameter<WhenActive> lines{ "Lines", WhenActive::Selected };
						PARAM_DECLARE("Debug draw", enabled, lines);
					} debugDraw;

					struct : ofParameterGroup {
						ofParameter<bool> enabled{ "Enabled", false };
						ofParameter<float> interval{ "Interval [s]", 5 };
						PARAM_DECLARE("Schedule", enabled, interval);
					} schedule;

					PARAM_DECLARE("Draw Moon", live, onMoonChange, errorIfOutsideRange, resolution, debugDraw, schedule);
				} parameters;

				struct Preview {
					ofPolyline polyline;
					ofColor color;
				};

				vector<Preview> previews;

				bool moonIsNew = false;
				bool moonIsNewNotify = false;

				struct {
					chrono::system_clock::time_point lastUpdate = chrono::system_clock::now();
				} schedule;
			};
		}
	}
}
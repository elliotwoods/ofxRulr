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

				void drawLasers();
			protected:
				struct : ofParameterGroup {
					ofParameter<WhenActive> live{ "Live", WhenActive::Never };
					ofParameter<float> resolution{ "Resolution", 100, 10, 1000 };

					struct : ofParameterGroup {
						ofParameter<bool> enabled{ "Enabled", true };
						ofParameter<WhenActive> lines{ "Lines", WhenActive::Selected };
						PARAM_DECLARE("Debug draw", enabled, lines);
					} debugDraw;

					PARAM_DECLARE("Draw Moon", live, resolution, debugDraw);
				} parameters;

				struct Preview {
					ofPolyline polyline;
					ofColor color;
				};

				vector<Preview> previews;
			};
		}
	}
}
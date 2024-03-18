#pragma once

#include "ofxRulr/Nodes/Base.h"
#include "ofxOsc.h"
#include "ofxRulr/Data/Dosirak/Curve.h"

namespace ofxRulr {
	namespace Nodes {
		namespace Dosirak {
			class DrawCurves : public Nodes::Base {
			public:
				DrawCurves();
				string getTypeName() const override;
				void init();
				void update();

				void populateInspector(ofxCvGui::InspectArguments&);

				void drawCurves();
			protected:

				struct Parameters : ofParameterGroup {
					ofParameter<bool> onNewFrame{ "onNewFrame", true };
					PARAM_DECLARE("DrawCurves", onNewFrame);
				} parameters;

				bool needsDrawCurves = false;
			};
		}
	}
}
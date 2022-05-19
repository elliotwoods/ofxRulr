#pragma once

#include "ofxRulr.h"

namespace ofxRulr {
	namespace Nodes {
		namespace AnotherMoon {
			class RemoteControl : public Nodes::Base {
			public:
				RemoteControl();
				string getTypeName() const override;

				void init();
				void populateInspector(ofxCvGui::InspectArguments&);
				
				ofxCvGui::PanelPtr getPanel() override;

				void homeAll();
				void setRGBAll(float, float, float);
			protected:
				struct : ofParameterGroup {
					struct : ofParameterGroup {
						ofParameter<float> slowSpeed{ "Slow speed", 0.01, 0, 1 };
						ofParameter<float> fastSpeed{ "Fast speed", 1, 0, 2 };
						PARAM_DECLARE("Adjust", slowSpeed, fastSpeed);
					} adjust;

					struct : ofParameterGroup {
						struct : ofParameterGroup {
							ofParameter<float> red{ "Red", 0.0f, 0.0f, 1.0f };
							ofParameter<float> green{ "Green", 0.0f, 0.0f, 1.0f };
							ofParameter<float> blue{ "Blue", 0.0f, 0.0f, 1.0f };
							PARAM_DECLARE("Color", red, green, blue);
						} color;
						PARAM_DECLARE("Set values", color);
					} setValues;

					PARAM_DECLARE("RemoteControl", adjust, setValues);
				} parameters;

				ofxCvGui::PanelPtr panel;
			};
		}
	}
}
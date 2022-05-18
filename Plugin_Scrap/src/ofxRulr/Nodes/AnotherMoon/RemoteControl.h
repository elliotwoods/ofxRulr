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
			protected:
				struct : ofParameterGroup {
					struct : ofParameterGroup {
						ofParameter<float> slowSpeed{ "Slow speed", 0.01, 0, 1 };
						ofParameter<float> fastSpeed{ "Fast speed", 1, 0, 2 };
						PARAM_DECLARE("Adjust", slowSpeed, fastSpeed);
					} adjust;

					PARAM_DECLARE("RemoteControl", adjust);
				} parameters;

				ofxCvGui::PanelPtr panel;
			};
		}
	}
}
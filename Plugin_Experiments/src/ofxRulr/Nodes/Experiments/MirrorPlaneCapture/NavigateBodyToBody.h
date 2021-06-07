#pragma once

#include "pch_Plugin_Experiments.h"

namespace ofxRulr {
	namespace Nodes {
		namespace Experiments {
			namespace MirrorPlaneCapture {
				class NavigateBodyToBody : public Nodes::Base {
				public:
					NavigateBodyToBody();
					string getTypeName() const override;

					void init();

					void populateInspector(ofxCvGui::InspectArguments&);

					void navigate();
				protected:
					struct : ofParameterGroup {
						ofParameter<int> maxIterations{ "Max iterations", 1000 };
						ofParameter<bool> printReport{ "Print report", true };
						ofParameter<bool> pushValues{ "Push values", true };
						ofParameter<bool> throwIfOutsideRange{ "Throw if outside range", false };
						PARAM_DECLARE("NavigateBodyToBody", maxIterations, printReport, pushValues, throwIfOutsideRange);
					} parameters;
				};
			}
		}
	}
}
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
						PARAM_DECLARE("NavigateBodyToBody", maxIterations, printReport);
					} parameters;
				};
			}
		}
	}
}
#pragma once

#include "pch_Plugin_Experiments.h"

namespace ofxRulr {
	namespace Nodes {
		namespace Experiments {
			namespace MirrorPlaneCapture {
				class NavigateToHalo : public Nodes::Base {
				public:
					NavigateToHalo();
					string getTypeName() const override;

					void init();
					void update();

					void populateInspector(ofxCvGui::InspectArguments&);

					void navigate();
				protected:
					struct : ofParameterGroup {
						struct : ofParameterGroup {
							ofParameter<bool> enabled{"Enabled", false};
							ofParameter<float> period{ "Period (s)", 10.0f };
							PARAM_DECLARE("Schedule", enabled, period);
						} schedule;

						struct : ofParameterGroup {
							ofParameter<int> maxIterations{"Max iterations", 5000};
							ofParameter<float> functionTolerance{"Function tolerance", 1e-7};
							ofParameter<bool> printReport{"Print report", false};
							PARAM_DECLARE("Solver", maxIterations, functionTolerance, printReport);
						} solver;
						
						PARAM_DECLARE("NavigateToHalo", schedule, solver);
					} parameters;

					chrono::system_clock::time_point lastUpdateTime = chrono::milliseconds(0);
				};
			}
		}
	}
}
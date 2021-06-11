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
					void drawWorldStage();

					void populateInspector(ofxCvGui::InspectArguments&);

					void navigate();

					void setAlternateTangents();

				protected:
					struct : ofParameterGroup {
						ofParameter<bool> onHaloChange{ "On halo change", true };

						struct : ofParameterGroup {
							ofParameter<bool> enabled{"Enabled", false};
							ofParameter<float> period{ "Period (s)", 10.0f, 1, 360 };
							PARAM_DECLARE("Schedule", enabled, period);
						} schedule;

						struct : ofParameterGroup {
							ofParameter<int> maxIterations{"Max iterations", 5000};
							ofParameter<float> functionTolerance{"Function tolerance", 1e-7};
							ofParameter<bool> printReport{"Print report", false};
							PARAM_DECLARE("Solver", maxIterations, functionTolerance, printReport);
						} solver;

						struct : ofParameterGroup {
							ofParameter<bool> targets{ "Targets", true };
							ofParameter<bool> lines{ "Lines", true };
							ofParameter<bool> solarIncidentVectors{ "Solar incident vectors", true };
							PARAM_DECLARE("Draw", targets, lines, solarIncidentVectors);
						} draw;
						
						PARAM_DECLARE("NavigateToHalo", onHaloChange, schedule, solver, draw);
					} parameters;

					chrono::system_clock::time_point lastUpdateTime{ chrono::milliseconds{ 0 } };

					struct CachedTarget {
						glm::vec3 heliostatPosition;
						glm::vec3 targetPosition;
						glm::vec3 solarIncidentVector;
					};
					map<string, CachedTarget> targetsCache;
				};
			}
		}
	}
}
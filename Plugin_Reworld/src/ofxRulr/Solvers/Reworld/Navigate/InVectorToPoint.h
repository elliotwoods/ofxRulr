#pragma once

#include "ofxRulr/Models/Reworld/Module.h"
#include "Result.h"

namespace ofxRulr {
	namespace Solvers {
		namespace Reworld {
			namespace Navigate {
				class InVectorToPoint {
				public:
					static ofxCeres::SolverSettings defaultSolverSettings();

					static Result solve(const Models::Reworld::Module<float>& module
						, const Models::Reworld::Module<float>::AxisAngles& initialAxisAngles
						, const glm::vec3& inVector
						, const glm::vec3& point
						, const ofxCeres::SolverSettings & = defaultSolverSettings());
				};
			}

		}
	}
}
#pragma once

#include "ofxRulr/Models/Reworld/Module.h"
#include "Result.h"

namespace ofxRulr {
	namespace Solvers {
		namespace Reworld {
			namespace Navigate {
				class PointToPoint {
				public:
					static ofxCeres::SolverSettings defaultSolverSettings();

					static Result solve(const Models::Reworld::Module<float>& module
						, const Models::Reworld::AxisAngles<float>& initialAxisAngles
						, const glm::vec3& point1
						, const glm::vec3& point2
						, const ofxCeres::SolverSettings& = defaultSolverSettings());
				};
			}
			
		}
	}
}
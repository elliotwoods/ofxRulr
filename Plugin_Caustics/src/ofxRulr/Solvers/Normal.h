#pragma once
#include "ofxCeres.h"

namespace ofxRulr {
	namespace Solvers {
		class Normal {
		public:
			typedef ofxCeres::Result<glm::vec3> Result;

			static ofxCeres::SolverSettings getDefaultSolverSettings();

			static Result solve(const glm::vec3 & incident
				, const glm::vec3 & refracted
				, float exitIORvsIncidentIOR
				, const ofxCeres::SolverSettings& solverSettings);
		};
	}
}
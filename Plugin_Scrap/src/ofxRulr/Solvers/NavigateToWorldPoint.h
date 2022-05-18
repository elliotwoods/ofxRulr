#pragma once

#include "ofxCeres.h"
#include "ofxRulr/Models/LaserProjector.h"

namespace ofxRulr {
	namespace Solvers {
		class NavigateToWorldPoint : ofxCeres::Models::Base
		{
		public:
			struct Solution {
				glm::vec2 point;
			};

			static ofxCeres::SolverSettings defaultSolverSettings();

			typedef ofxCeres::Result<Solution> Result;

			static Result solve(const glm::vec3& worldPoint
				, const Models::LaserProjector& laserProjector
				, const glm::vec2& initialGuess
				, const ofxCeres::SolverSettings& solverSettings = defaultSolverSettings());
		};
	}
}
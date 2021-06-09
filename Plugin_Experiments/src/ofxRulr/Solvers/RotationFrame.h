#pragma once

#include "ofxCeres.h"
#include "ofxRay.h"

namespace ofxRulr {
	namespace Solvers {
		class RotationFrame : ofxCeres::Models::Base
		{
		public:
			struct Solution {
				glm::quat rotation;
			};

			static ofxCeres::SolverSettings defaultSolverSettings();

			typedef ofxCeres::Result<Solution> Result;

			static Result solve(const vector<glm::vec3> & preRotation
				, const vector<glm::vec3>& postRotation
				, const ofxCeres::SolverSettings& solverSettings = defaultSolverSettings());
		};
	}
}
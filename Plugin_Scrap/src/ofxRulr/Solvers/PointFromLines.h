#pragma once

#include "ofxCeres.h"
#include "ofxRulr/Models/Line.h"

namespace ofxRulr {
	namespace Solvers {
		class PointFromLines : ofxCeres::Models::Base
		{
		public:
			struct Solution {
				glm::vec2 point;
			};

			static ofxCeres::SolverSettings defaultSolverSettings();

			typedef ofxCeres::Result<Solution> Result;

			static Result solve(const vector<Models::Line>& lines
				, const ofxCeres::SolverSettings& solverSettings = defaultSolverSettings());
		};
	}
}
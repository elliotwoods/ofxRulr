#pragma once

#include "ofxRulr/Models/IntegratedSurface.h"

namespace ofxRulr {
	namespace Solvers {
		class NormalsSurface : ofxCeres::Models::Base {
		public:
			struct Solution {
				Models::IntegratedSurface surface;
			};
			typedef ofxCeres::Result<Solution> Result;

			static ofxCeres::SolverSettings getDefaultSolverSettings();

			static Result solve(const Models::IntegratedSurface& surface
				, const ofxCeres::SolverSettings& solverSettings);
		};
	}
}
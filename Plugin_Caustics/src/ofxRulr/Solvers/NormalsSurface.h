#pragma once

#include "ofxRulr/Models/Surface.h"

namespace ofxRulr {
	namespace Solvers {
		class NormalsSurface : ofxCeres::Models::Base {
		public:
			struct Solution {
				Models::Surface surface;
			};
			typedef ofxCeres::Result<Solution> Result;

			static ofxCeres::SolverSettings getDefaultSolverSettings();

			static vector<double*> initParameters(const Models::Surface_<double>& surface);

			static Result solveUniversal(const Models::Surface& surface
				, const ofxCeres::SolverSettings& solverSettings);

			static Result solveByIndividual(const Models::Surface& surface
				, const ofxCeres::SolverSettings& solverSettings
				, bool edgesOnly = false);

			static Result solveSection(const Models::Surface& surface
				, const ofxCeres::SolverSettings& solverSettings
				, const Models::SurfaceSectionSettings& surfaceSectionSettings);
		};
	}
}
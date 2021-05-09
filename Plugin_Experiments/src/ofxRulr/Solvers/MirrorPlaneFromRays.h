#pragma once

#include "ofxCeres.h"
#include "ofxRay.h"

namespace ofxRulr {
	namespace Solvers {
		class MirrorPlaneFromRays : ofxCeres::Models::Base
		{
		public:
			struct Solution {
				ofxRay::Plane plane;
			};

			static ofxCeres::SolverSettings defaultSolverSettings();

			typedef ofxCeres::Result<Solution> Result;

			static Result solve(const vector<ofxRay::Ray> & cameraRays
				, const vector<glm::vec3>& worldPoints
				, const ofxCeres::SolverSettings& solverSettings = defaultSolverSettings());

			static float getResidual(const glm::vec4 & planeABCD
				, const ofxRay::Ray& cameraRay
				, const glm::vec3 worldPoint);
		};
	}
}
#pragma once

#include "ofxRulr/Models/Camera.h"

namespace ofxRulr {
	namespace Solvers {
		/// <summary>
		/// We presume that we already have camera intrisincs
		/// and that image points are already undistorted.
		/// </summary>
		class BundleAdjustmentPoints {
		public:
			struct Image {
				int pointIndex;
				int viewIndex;
				glm::vec2 imagePoint;
			};

			struct Solution {
				vector<glm::vec3> worldPoints;
				vector<Models::Transform> cameraViewTransforms;
			};
			typedef ofxCeres::Result<Solution> Result;

			static ofxCeres::SolverSettings defaultSolverSettings();

			class Problem {
			public:
				Problem(size_t pointCount
					, size_t viewCount
					, const Models::Intrinsics& cameraIntrinsics);
				Problem(const Solution& initialSolution
					, const Models::Intrinsics& cameraIntrinsics);

				~Problem();

				void addImageConstraint(const Image&, bool applyWeightByDistanceFromImageCenter);
				void addSceneScaleConstraint(float maxRadius);
				void addSceneCenteredConstraint(const glm::vec3& sceneCenter);
				void addCameraZeroYawConstraint(int viewIndex);
				void addCameraZeroRollConstrant(int viewIndex);

				Result solve(const ofxCeres::SolverSettings&);
			protected:
				ceres::Problem problem;
				const Models::Intrinsics cameraIntrinsics;

				vector<double*> allWorldPointParameters;
				vector<double*> allViewTranslateParameters;
				vector<double*> allViewRotationParameters;
			};
		};
	}
}
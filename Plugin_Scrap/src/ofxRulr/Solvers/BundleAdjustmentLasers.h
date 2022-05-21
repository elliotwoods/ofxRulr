#pragma once

#include "ofxRulr/Models/Camera.h"
#include "ofxRulr/Models/LaserProjector.h"

namespace ofxRulr {
	namespace Solvers {
		class BundleAdjustmentLasers {
		public:
			struct Image {
				int cameraIndex;
				int laserProjectorIndex;
				Models::Line imageLine;
				glm::vec2 projectedPoint;

				string toString() const {
					stringstream ss;
					ss << "cameraIndex (" << this->cameraIndex << ") "
						<< "laserIndex (" << this->laserProjectorIndex << ") "
						<< "projectedPoint (" << this->projectedPoint << ")";
					return ss.str();
				}
			};

			struct Solution {
				vector<Models::Transform> cameraViewTransforms;
				vector<Models::LaserProjector> laserProjectors;
			};

			typedef ofxCeres::Result<Solution> Result;

			static ofxCeres::SolverSettings defaultSolverSettings();
			
			static void fillCameraParameters(const Models::Transform&
				, double* cameraTranslationParameters
				, double* cameraRotationParameters);

			static void fillLaserParameters(const Models::LaserProjector&
				, double* laserTranslationParameters
				, double* laserRotationParameters
				, double* laserFovParameters);

			class Problem {
			public:
				Problem(const Solution& initialSolution
					, const Models::Intrinsics& cameraIntrinsics);
				~Problem();

				void addLineImageObservation(const Image&);
				void addLaserLayoutScaleConstraint(float maxRadius);
				void addLaserLayoutCenteredConstraint(const glm::vec3& sceneCenter);
				void addCameraZeroYawConstraint(int viewIndex);
				void addCameraZeroRollConstrant(int viewIndex);
				void addLasersInPlaneConstraint(size_t plane);

				void setLaserPositionsFixed();
				void setLaserPositionsVariable();

				Result solve(const ofxCeres::SolverSettings&);
			protected:
				ceres::Problem problem;
				const Models::Intrinsics cameraIntrinsics;

				vector<double*> allCameraTranslationParameters;
				vector<double*> allCameraRotationParameters;
				vector<double*> allLaserTranslationParameters;
				vector<double*> allLaserRotationParameters;
				vector<double*> allLaserFovParameters;
			};

			static float getResidual(const Solution&
				, const Models::Intrinsics &
				, const Image&);
		};
	}
}
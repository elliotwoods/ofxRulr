#pragma once
#include "ofxCeres.h"
#include <glm/glm.hpp>

namespace ofxRulr {
	namespace Solvers {
		class MarkerProjections {
		public:
			struct Solution {
				struct Transform {
					glm::vec3 translation;
					glm::vec3 rotation;
				};

				vector<Transform> objects;

				// Note that view transforms are inverted
				vector<Transform> views;

				vector<float> reprojectionErrorPerImage;
			};

			struct Image {
				int viewIndex;
				int objectIndex;
				vector<glm::vec2> imagePointsUndistorted;
			};

			typedef ofxCeres::Result<Solution> Result;

			static ofxCeres::SolverSettings defaultSolverSettings();

			static Result solve(int cameraWidth
				, int cameraHeight
				, const glm::mat4 & cameraProjectionMatrix
				, const std::vector<vector<glm::vec3>> & objectPoints
				, const vector<Image>& images
				, const vector<int>& fixedObjectIndices
				, const Solution& initialSolution
				, const ofxCeres::SolverSettings& solverSettings = defaultSolverSettings());

			static Solution::Transform getTransform(const glm::mat4&);
			static glm::mat4 getTransform(const Solution::Transform&);
		};

	}
}
#pragma once

#include "ofxCeres.h"
#include "ofxRulr/Models/Line.h"

namespace ofxRulr {
	namespace Solvers {
		class LineToImage : ofxCeres::Models::Base
		{
		public:
			struct Solution {
				Models::Line line;
				float meanResidual;
			};

			static ofxCeres::SolverSettings defaultSolverSettings();

			typedef ofxCeres::Result<Solution> Result;

			static Result solve(const cv::Mat& image
				, uint8_t minimumValue
				, const ofxCeres::SolverSettings& solverSettings = defaultSolverSettings());

			static float getResidual(const vector<glm::vec2>& points
				, const vector<float>& weights
				, const Solution&);

			static float getMeanResidual(const vector<glm::vec2>& points
				, const vector<float>& weights
				, const Solution&);

			static void imageToData(const cv::Mat& image
				, uint8_t minimumValue
				, vector<glm::vec2> & points
				, vector<float> & weights);
		};
	}
}
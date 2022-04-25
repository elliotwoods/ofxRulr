#pragma once

#include "ofxCeres.h"
#include "LineToImage.h"
#include "ofxRulr/Nodes/Item/Camera.h"

namespace ofxRulr {
	namespace Solvers {
		class LinesWithCommonPoint : ofxCeres::Models::Base
		{
		public:
			struct Solution {
				vector<LineToImage::Line> lines;
				vector<bool> linesValid;
				glm::vec2 point;
			};

			struct CameraImagePoints {
				cv::Mat thresholdedImage;
				vector<glm::vec2> points;
				vector<float> weights;
			};

			static ofxCeres::SolverSettings defaultSolverSettings();

			typedef ofxCeres::Result<Solution> Result;

			static Result solve(const vector<CameraImagePoints> & images
				, const float& distanceThreshold
				, const float& minMeanPixelValueOnLine
				, const ofxCeres::SolverSettings& solverSettings = defaultSolverSettings());

			static CameraImagePoints getCameraImagePoints(const cv::Mat& differenceImage
				, shared_ptr<Nodes::Item::Camera>
				, float normalizePercentile
				, float differenceThreshold
				, cv::Mat& preview = cv::Mat());
		};
	}
}
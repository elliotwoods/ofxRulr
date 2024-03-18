#pragma once

#include "ofxCeres.h"
#include "ofxRulr/Models/Line.h"
#include "ofxRulr/Nodes/Item/Camera.h"

namespace ofxRulr {
	namespace Solvers {
		class LinesWithCommonPoint : ofxCeres::Models::Base
		{
		public:
			struct Solution {
				vector<Models::Line> lines;
				vector<bool> linesValid;
				glm::vec2 point;

				/// <summary>
				/// This preview is only available when solveData.debug.previewEnabled = true
				/// </summary>
				cv::Mat preview;

				vector<float> meanPixelValuesOnLine;
			};

			struct CameraImagePoints {
				vector<glm::vec2> points;
				vector<float> weights;

				/// <summary>
				/// This image is used later for testing solved lines
				/// </summary>
				cv::Mat thresholdedImage;
			};

			static ofxCeres::SolverSettings defaultSolverSettings();

			typedef ofxCeres::Result<Solution> Result;

			struct SolveData {
				vector<cv::Mat> onImages;
				vector<cv::Mat> offImages;

				shared_ptr<Nodes::Item::Camera> cameraNode;

				float normalizePercentile;
				float differenceThreshold;
				float distanceThreshold;
				float minMeanPixelValueOnLine;
				bool useAlternativeSolve;

				struct {
					bool enabled;
					int threshold;
					int erosionSteps;
					int dilationSteps;
					int dilationSize;
				} ignoreAroundBrightSpots;

				struct {
					bool enabled;
					int diameter;
				} maxFeatureDiameter;

				struct {
					bool previewEnabled = false;
					bool previewPopup = false;

					/// <summary>
					/// These values should exactly match dimensions of onIamges and offImages
					/// </summary>
					cv::Size previewSize{ 8192, 5464 };
					filesystem::path directory;
				} debug;
			};

			/// <summary>
			/// Solve a set of converging 2D lines, e.g. for a set of beam captures for a single laser in a single camera position
			/// </summary>
			/// <param name="solveData">All inputs to solution</param>
			/// <param name="solverSettings">ofxCeres solve settings</param>
			/// <returns>Lines and point</returns>
			static Result solve(const SolveData& solveData
				, const ofxCeres::SolverSettings& solverSettings = defaultSolverSettings());

			/// <summary>
			/// Get the active image points in a single beam capture. During this step we undistort the image
			/// </summary>
			/// <param name="differenceImage">On image minus off image</param>
			/// <param name="solveData">Settings for solve</param>
			/// <returns>CameraImagePoints</returns>
			static CameraImagePoints getCameraImagePoints(const cv::Mat& differenceImage
				, const SolveData& solveData);

			/// <summary>
			/// Inner solve function. This is called by our other solve function
			/// </summary>
			static Result solve(const vector<CameraImagePoints> & images
				, const SolveData&
				, const ofxCeres::SolverSettings& solverSettings = defaultSolverSettings());
		};
	}
}
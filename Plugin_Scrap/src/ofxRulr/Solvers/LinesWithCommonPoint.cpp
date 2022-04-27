#include "pch_Plugin_Scrap.h"
#include "LinesWithCommonPoint.h"
#include "PointFromLines.h"
#include <opencv2/opencv.hpp>

#include "ofxRulr/Nodes/Item/Camera.h"

typedef ofxRulr::Models::Line Line;

struct LinesWithCommonPointCost
{
	LinesWithCommonPointCost(const glm::vec2& point
		, const float& weight)
		: point(point)
		, weight(weight)
	{

	}

	template<typename T>
	bool
		operator()(const T* const pointParameters
			, const T* const angleParameters
			, T* residuals) const
	{
		// Construct line from parameters
		ofxRulr::Models::Line_<T> line(
			glm::tvec2<T>(pointParameters[0], pointParameters[1])
			, angleParameters[0]);

		auto distance = line.distanceToPoint((glm::tvec2<T>) this->point);
		residuals[0] = (T)weight * distance;
		return true;
	}

	static ceres::CostFunction*
		Create(const glm::vec2& point
			, const float& weight)
	{
		return new ceres::AutoDiffCostFunction<LinesWithCommonPointCost, 1, 2, 1>(
			new LinesWithCommonPointCost(point, weight)
			);
	}

	const glm::vec2& point;
	const float& weight;
};

namespace ofxRulr {
	namespace Solvers {
		//----------
		ofxCeres::SolverSettings
			LinesWithCommonPoint::defaultSolverSettings()
		{
			ofxCeres::SolverSettings solverSettings;
			return solverSettings;
		}

		//----------
		LinesWithCommonPoint::Result
			LinesWithCommonPoint::solve(const SolveData& solveData
				, const ofxCeres::SolverSettings& solverSettings)
		{
			// Check input
			if (solveData.onImages.size() != solveData.offImages.size()) {
				throw(ofxRulr::Exception("LinesWithCommonPoint::solve : Mismatch between onImages and offImages"));
			}
			if (solveData.onImages.empty()) {
				throw(ofxRulr::Exception("LinesWithCommonPoint::solve : No image data"));
			}

			// Allocate the preview
			vector<cv::Mat> previewPlanes;
			const auto& previewEnabled = solveData.debug.previewEnabled;
			if(previewEnabled) {
				// Allocate 3 planes with full resolution
				previewPlanes.push_back(cv::Mat(solveData.debug.previewSize, CV_8UC1));
				previewPlanes.push_back(cv::Mat(solveData.debug.previewSize, CV_8UC1));
				previewPlanes.push_back(cv::Mat(solveData.debug.previewSize, CV_8UC1));
			}

			// Gather CameraImagePoints
			size_t size = solveData.onImages.size();
			vector<Solvers::LinesWithCommonPoint::CameraImagePoints> images;
			for (size_t i = 0; i < size; i++) {
				const auto& onImageRaw = solveData.onImages[i];
				const auto& offImageRaw = solveData.offImages[i];

				// Get the positive difference
				cv::Mat differenceRaw;
				cv::subtract(onImageRaw, offImageRaw, differenceRaw);

				// Get the camera image points for this image
				images.push_back(Solvers::LinesWithCommonPoint::getCameraImagePoints(differenceRaw, solveData));

				// Add the difference to the preview
				if (previewEnabled) {
					cv::Mat differenceUndistorted;

					// Undistort the image data for the preview
					cv::undistort(differenceRaw
						, differenceUndistorted
						, solveData.cameraNode->getCameraMatrix()
						, solveData.cameraNode->getDistortionCoefficients());

					// Add it into channel 1 (green channel)
					cv::addWeighted(previewPlanes[1], 1.0
						, differenceUndistorted, 1.0 / (double) size
						, 1.0
						, previewPlanes[1]);
				}
			}

			// Solve lines for the whole laser using a common convergence point
			auto result = Solvers::LinesWithCommonPoint::solve(images
				, solveData
				, solverSettings);

			// Draw lines into preview and merge the preview channels
			if (previewEnabled) {
				// Draw the lines onto red channel with indices
				{
					int lineIndex = 0;
					for (const auto& line : result.solution.lines) {
						line.drawOnImage(previewPlanes[0]);

						// draw the text
						{
							auto meetsEdges = line.getImageEdgeIntersects(solveData.debug.previewSize);
							if (meetsEdges.size() >= 2) {
								auto firstEdge = meetsEdges.begin();
								auto secondEdge = firstEdge++;
								auto midLine = (*firstEdge + *secondEdge) / 2.0f;

								cv::putText(previewPlanes[0]
									, "[" + ofToString(lineIndex) + "]"
									, ofxCv::toCv(midLine)
									, cv::FONT_HERSHEY_PLAIN
									, 5
									, cv::Scalar(255)
									, 8);
							}

						}


						lineIndex++;
					}
				}

				// Take the preview returned from the solve and use this as plane 2
				previewPlanes[2] = result.solution.preview;

				cv::putText(previewPlanes[0]
					, "Lines from complete solve"
					, cv::Point(10, 100)
					, cv::FONT_HERSHEY_PLAIN
					, 8
					, cv::Scalar(255));

				//normalise this plane because it's dim (before writing on it)
				cv::normalize(previewPlanes[1]
					, previewPlanes[1]
					, 0
					, 255
					, cv::NORM_MINMAX);
				cv::putText(previewPlanes[1]
					, "Difference"
					, cv::Point(10, 200)
					, cv::FONT_HERSHEY_PLAIN
					, 8
					, cv::Scalar(255));
				cv::putText(previewPlanes[2]
					, "Lines from initial OpenCV fitline"
					, cv::Point(10, 300)
					, cv::FONT_HERSHEY_PLAIN
					, 8
					, cv::Scalar(255));

				// Create output preview
				cv::merge(previewPlanes, result.solution.preview);

				// Show preview
				if (solveData.debug.previewPopup) {
					ofxCv::Modals::WindowProperties windowProperties;
					windowProperties.normalizeColors = true;
					ofxCv::Modals::showImage(result.solution.preview
						, "After LinesWithCommonPoint solve"
						, windowProperties);
				}
			}

			// Return result
			return result;
		}

		//---------
		LinesWithCommonPoint::CameraImagePoints
			LinesWithCommonPoint::getCameraImagePoints(const cv::Mat& differenceImageRaw
				, const SolveData& solveData)
		{
			// Undistort the difference image for first pass
			cv::Mat differenceImage;
			{
				cv::undistort(differenceImageRaw
					, differenceImage
					, solveData.cameraNode->getCameraMatrix()
					, solveData.cameraNode->getDistortionCoefficients());
			}

			// Take local difference on both raw and undistorted
			cv::Mat localDifference;
			cv::Mat localDifferenceRaw;
			{
				{
					cv::Mat blurred;
					cv::blur(differenceImageRaw, blurred, cv::Size(64, 64));

					cv::addWeighted(differenceImageRaw, 1
						, blurred, -1
						, 0
						, localDifferenceRaw);
				}

				{
					cv::Mat blurred;
					cv::blur(differenceImage, blurred, cv::Size(64, 64));

					cv::addWeighted(differenceImage, 1
						, blurred, -1
						, 0
						, localDifference);
				}
			}

			// Normalize the images
			cv::Mat normalizedImage;
			cv::Mat normalizedImageRaw;
			{
				// Sort pixel values
				vector<uint8_t> pixelValues;
				auto data = localDifference.data;
				auto size = localDifference.cols * localDifference.rows;
				for (size_t i = 0; i < size; i += 4) {
					pixelValues.push_back(data[i]);
				}
				sort(pixelValues.begin(), pixelValues.end());

				// Get the percentile value
				auto maxValue = pixelValues.at((size_t) ((double)(pixelValues.size() - 1) * (1.0 - solveData.normalizePercentile)));
				float normFactor = 127.0f / (float)maxValue;
				cout << "Max pixel value before normalisation : " << (int) maxValue << endl;

				// Apply norm factor
				localDifferenceRaw.convertTo(normalizedImageRaw
					, differenceImageRaw.type()
					, normFactor);
				localDifference.convertTo(normalizedImage
					, differenceImage.type()
					, normFactor);
			}

			// Threshold the raw image and get the undistorted points
			cv::Mat thresholdedRaw;
			CameraImagePoints cameraImagePoints;
			{
				cv::threshold(normalizedImageRaw
					, thresholdedRaw
					, solveData.differenceThreshold
					, 255
					, cv::THRESH_BINARY);

				// Get the non-zero points in raw image
				vector<cv::Point2i> pointsInt;
				cv::findNonZero(thresholdedRaw, pointsInt);

				// Gather the points
				for (const auto& pointInt : pointsInt) {
					cameraImagePoints.points.emplace_back(pointInt.x, pointInt.y);
					cameraImagePoints.weights.push_back(differenceImageRaw.at<uint8_t>(pointInt.y, pointInt.x));
				}

				// Undistort the points
				ofxCv::toCv(cameraImagePoints.points) = ofxCv::undistortImagePoints(ofxCv::toCv(cameraImagePoints.points)
					, solveData.cameraNode->getCameraMatrix()
					, solveData.cameraNode->getDistortionCoefficients());
			}

			// Thereshold the undistorted image for preview purposes
			cv::Mat thresholded;
			{
				cv::threshold(normalizedImage
					, thresholded
					, solveData.differenceThreshold
					, 255
					, cv::THRESH_BINARY);
			}

			cameraImagePoints.thresholdedImage = thresholded;

			return cameraImagePoints;
		}

		//----------
		LinesWithCommonPoint::Result
			LinesWithCommonPoint::solve(const vector<CameraImagePoints>& cameraImagePointSets
				, const SolveData& solveData
				, const ofxCeres::SolverSettings& solverSettings)
		{
			// Check we have data
			if (cameraImagePointSets.empty()) {
				throw(ofxRulr::Exception("No images"));
			}

			// Initialise a preview
			cv::Mat preview;
			if (solveData.debug.previewEnabled) {
				// This needs to be 1 plane because externally this will be merged 
				preview = cv::Mat(solveData.debug.previewSize, CV_8UC1);
			}

			// Initialise parameters
			glm::tvec2<double> pointParameters(0.0, 0.0);
			vector<vector<double>> angleParameters(cameraImagePointSets.size(), vector<double>(1, 0.0));
			vector<bool> passTest;
			vector<Line> firstLines;
			{

				for (int i = 0; i < cameraImagePointSets.size(); i++) {
					cv::Vec4f openCVLine;

					vector<cv::Point2i> intPoints;
					for (const auto& point : cameraImagePointSets[i].points) {
						intPoints.push_back(ofxCv::toCv(point));
					}

					cv::fitLine(intPoints
						, openCVLine
						, cv::DIST_HUBER
						, 0
						, 0.01
						, 0.01);

					auto line = Line(openCVLine);

					auto meanPixelValueOnLine = line.meanMaskedValue(cameraImagePointSets[i].thresholdedImage);
					passTest.push_back(meanPixelValueOnLine >= solveData.minMeanPixelValueOnLine);

					firstLines.push_back(line);

					if (solveData.debug.previewEnabled) {
						line.drawOnImage(preview);
					}
				}

				// Guess convergence point using lines found with opencv
				auto findPointResult = PointFromLines::solve(firstLines);
				pointParameters.x = findPointResult.solution.point.x;
				pointParameters.y = findPointResult.solution.point.y;

				// First guess of angle parameters = (opencv found line's start point) - (guessed convergence point)
				for (int i = 0; i < cameraImagePointSets.size(); i++) {
					angleParameters[i][0] = atan2((double)firstLines[i].s.y - pointParameters.y
						, (double)firstLines[i].s.x - pointParameters.x);
				}

				// Show initial guess for lines and convergence
				if(solveData.debug.previewEnabled && solveData.debug.previewPopup) {
					// Draw convergence point
					cv::circle(preview
						, cv::Point(pointParameters.x, pointParameters.y)
						, 20
						, cv::Scalar(255)
						, 1
						, cv::LineTypes::LINE_8);

					ofxCv::Modals::showImage(preview, "Lines and convergence before fine solve");
				}
			}

			//--
			// Fit the lines for all beam captures in this laser (in this camera)
			//--
			//
			ceres::Problem problem;

			// Add data to problem (but trim to only keep points that are already within max distance)
			{
				for (size_t i = 0; i < cameraImagePointSets.size(); i++) {
					if (!passTest[i]) {
						continue;
					}

					auto& points = cameraImagePointSets[i].points;
					auto& weights = cameraImagePointSets[i].weights;

					for (size_t j = 0; j < points.size(); j++) {
						if (firstLines[i].distanceToPoint(points[j]) <= solveData.distanceThreshold) {
							problem.AddResidualBlock(LinesWithCommonPointCost::Create(points[j], weights[j])
								, NULL
								, &pointParameters[0]
								, angleParameters[i].data());
						}
					}
				}
			}

			// Solve the problem;
			if (solverSettings.printReport) {
				cout << "Solve LineswithCommonPoint" << endl;
			}
			ceres::Solver::Summary summary;
			ceres::Solve(solverSettings.options
				, &problem
				, &summary);

			if (solverSettings.printReport) {
				cout << summary.FullReport() << endl;
			}
			//
			//--

			{
				Result result(summary);
				result.solution.point = (glm::vec2)pointParameters;
				result.solution.linesValid = passTest;

				for (int i = 0; i < cameraImagePointSets.size(); i++) {
					Line line(result.solution.point, (float)angleParameters[i][0]);
					result.solution.lines.push_back(line);
				}

				result.solution.preview = preview;

				return result;
			}
		}
	}
}
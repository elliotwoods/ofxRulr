#include "pch_Plugin_Scrap.h"
#include "LinesFromPoint.h"
#include "PointFromLines.h"
#include <opencv2/opencv.hpp>

typedef ofxRulr::Solvers::LineToImage::Line Line;

struct LinesFromPointCost
{
	LinesFromPointCost(const glm::vec2& point
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
		ofxRulr::Solvers::LineToImage::Line_<T> line(
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
		return new ceres::AutoDiffCostFunction<LinesFromPointCost, 1, 2, 1>(
			new LinesFromPointCost(point, weight)
			);
	}

	const glm::vec2& point;
	const float& weight;
};

namespace ofxRulr {
	namespace Solvers {
		//----------
		ofxCeres::SolverSettings
			LinesFromPoint::defaultSolverSettings()
		{
			ofxCeres::SolverSettings solverSettings;
			return solverSettings;
		}

		//----------
		LinesFromPoint::Result
			LinesFromPoint::solve(const vector<CameraImagePoints>& cameraImagePoints
				, const float& distanceThreshold
				, const float& minMeanPixelValueOnLine
				, const ofxCeres::SolverSettings& solverSettings)
		{
			if (cameraImagePoints.empty()) {
				throw(ofxRulr::Exception("No images"));
			}

			// Initialise parameters
			glm::tvec2<double> pointParameters( 0.0, 0.0 );
			vector<vector<double>> angleParameters(cameraImagePoints.size(), vector<double>(1, 0.0));
			vector<bool> passTest;
			vector<Line> firstLines;
			{

				for (int i = 0; i < cameraImagePoints.size(); i++) {
					cv::Vec4f openCVLine;

					vector<cv::Point2i> intPoints;
					for (const auto& point : cameraImagePoints[i].points) {
						intPoints.push_back(ofxCv::toCv(point));
					}

					cv::fitLine(intPoints
						, openCVLine
						, cv::DIST_HUBER
						, 0
						, 0.01
						, 0.01);

					firstLines.emplace_back(openCVLine);

					auto meanPixelValueOnLine = firstLines.back().meanMaskedValue(cameraImagePoints[i].thresholdedImage);
					passTest.push_back(meanPixelValueOnLine >= minMeanPixelValueOnLine);
				}

				// Guess convergence point using lines found with opencv
				auto findPointResult = PointFromLines::solve(firstLines);
				pointParameters.x = findPointResult.solution.point.x;
				pointParameters.y = findPointResult.solution.point.y;

				// First guess of angle parametrs is the (opencv found line's start point) - (guessed convergence point)
				for (int i = 0; i < cameraImagePoints.size(); i++) {
					angleParameters[i][0] = atan2((double) firstLines[i].s.y - pointParameters.y
						, (double) firstLines[i].s.x - pointParameters.x);
				}
			}

			ceres::Problem problem;

			// Add data to problem (but trim to only keep points that are already within max distance)
			{
				for (size_t i = 0; i < cameraImagePoints.size(); i++) {
					if (!passTest[i]) {
						continue;
					}

					auto& points = cameraImagePoints[i].points;
					auto& weights = cameraImagePoints[i].weights;

					for (size_t j = 0; j < points.size(); j++) {
						if (firstLines[i].distanceToPoint(points[j]) <= distanceThreshold) {
							problem.AddResidualBlock(LinesFromPointCost::Create(points[j], weights[j])
								, NULL
								, &pointParameters[0]
								, angleParameters[i].data());
						}
					}
				}
			}

			// Solve the problem;
			ceres::Solver::Summary summary;
			ceres::Solve(solverSettings.options
				, &problem
				, &summary);

			if (solverSettings.printReport) {
				cout << summary.FullReport() << endl;
			}

			{
				Result result(summary);
				result.solution.point = (glm::vec2)pointParameters;
				result.solution.linesValid = passTest;

				for (int i = 0; i < cameraImagePoints.size(); i++) {
					LineToImage::Line line(result.solution.point, (float)angleParameters[i][0]);
					result.solution.lines.push_back(line);
				}

				return result;
			}
		}

		//---------
		LinesFromPoint::CameraImagePoints
			LinesFromPoint::getCameraImagePoints(const cv::Mat& differenceImage
				, float normalizePercentile
				, float differenceThreshold
				, cv::Mat& preview)
		{
			// Take local difference
			cv::Mat localDifference;
			{
				cv::Mat blurred;
				cv::blur(differenceImage, blurred, cv::Size(64, 64));

				cv::addWeighted(differenceImage, 1
					, blurred, -1
					, 0, localDifference);
			}

			// Normalize the image 
			cv::Mat normalizedImage;
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
				auto maxValue = pixelValues.at((size_t) ((float)(pixelValues.size() - 1) * (1.0f - normalizePercentile)));
				float normFactor = 127.0f / (float)maxValue;

				// Apply norm factor
				localDifference.convertTo(normalizedImage
					, differenceImage.type()
					, normFactor);

				cout << "Max value : " << (int) maxValue << endl;
			}

			// Threshold the image and get the points
			cv::Mat thresholded;
			CameraImagePoints cameraImagePoints;
			{
				cv::threshold(normalizedImage
					, thresholded
					, differenceThreshold
					, 255
					, cv::THRESH_BINARY);
				
				vector<cv::Point2i> pointsInt;
				cv::findNonZero(thresholded, pointsInt);
				for (const auto& pointInt : pointsInt) {
					cameraImagePoints.points.emplace_back(pointInt.x, pointInt.y);
					cameraImagePoints.weights.push_back(differenceImage.at<uint8_t>(pointInt.y, pointInt.x));
				}
			}

			// Draw on preview if supplied
			if (!preview.empty()) {
				// Create the line image
				cv::addWeighted(preview, 1.0
					, thresholded, 0.1
					, 0.0
					, preview);
			}

			cameraImagePoints.thresholdedImage = thresholded;

			return cameraImagePoints;
		}
	}
}
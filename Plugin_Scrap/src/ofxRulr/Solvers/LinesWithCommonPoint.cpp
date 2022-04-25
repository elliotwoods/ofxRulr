#include "pch_Plugin_Scrap.h"
#include "LinesWithCommonPoint.h"
#include "PointFromLines.h"
#include <opencv2/opencv.hpp>

#include "ofxRulr/Nodes/Item/Camera.h"

typedef ofxRulr::Solvers::LineToImage::Line Line;

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
			LinesWithCommonPoint::solve(const vector<CameraImagePoints>& cameraImagePointSets
				, const float& distanceThreshold
				, const float& minMeanPixelValueOnLine
				, const ofxCeres::SolverSettings& solverSettings)
		{
			if (cameraImagePointSets.empty()) {
				throw(ofxRulr::Exception("No images"));
			}

			// Initialise parameters
			glm::tvec2<double> pointParameters( 0.0, 0.0 );
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

					firstLines.emplace_back(openCVLine);

					auto meanPixelValueOnLine = firstLines.back().meanMaskedValue(cameraImagePointSets[i].thresholdedImage);
					passTest.push_back(meanPixelValueOnLine >= minMeanPixelValueOnLine);
				}

				// Guess convergence point using lines found with opencv
				auto findPointResult = PointFromLines::solve(firstLines);
				pointParameters.x = findPointResult.solution.point.x;
				pointParameters.y = findPointResult.solution.point.y;

				// First guess of angle parameters = (opencv found line's start point) - (guessed convergence point)
				for (int i = 0; i < cameraImagePointSets.size(); i++) {
					angleParameters[i][0] = atan2((double) firstLines[i].s.y - pointParameters.y
						, (double) firstLines[i].s.x - pointParameters.x);
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
						if (firstLines[i].distanceToPoint(points[j]) <= distanceThreshold) {
							problem.AddResidualBlock(LinesWithCommonPointCost::Create(points[j], weights[j])
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
			//
			//--

			{
				Result result(summary);
				result.solution.point = (glm::vec2)pointParameters;
				result.solution.linesValid = passTest;

				for (int i = 0; i < cameraImagePointSets.size(); i++) {
					LineToImage::Line line(result.solution.point, (float)angleParameters[i][0]);
					result.solution.lines.push_back(line);
				}

				return result;
			}
		}

		//---------
		LinesWithCommonPoint::CameraImagePoints
			LinesWithCommonPoint::getCameraImagePoints(const cv::Mat& differenceImageRaw
				, shared_ptr<Nodes::Item::Camera> cameraNode
				, float normalizePercentile
				, float differenceThreshold
				, cv::Mat& preview)
		{
			// Undistort the camera
			cv::Mat differenceImage;
			{
				cv::undistort(differenceImageRaw
					, differenceImage
					, cameraNode->getCameraMatrix()
					, cameraNode->getDistortionCoefficients());
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
				auto maxValue = pixelValues.at((size_t) ((float)(pixelValues.size() - 1) * (1.0f - normalizePercentile)));
				float normFactor = 127.0f / (float)maxValue;

				// Apply norm factor
				localDifferenceRaw.convertTo(normalizedImageRaw
					, differenceImageRaw.type()
					, normFactor);
				localDifference.convertTo(normalizedImage
					, differenceImage.type()
					, normFactor);

				cout << "Max value : " << (int) maxValue << endl;
			}

			// Threshold the images and get the undistorted points
			cv::Mat thresholded;
			cv::Mat thresholdedRaw;
			CameraImagePoints cameraImagePoints;
			{
				cv::threshold(normalizedImageRaw
					, thresholdedRaw
					, differenceThreshold
					, 255
					, cv::THRESH_BINARY);

				cv::threshold(normalizedImage
					, thresholded
					, differenceThreshold
					, 255
					, cv::THRESH_BINARY);
				
				{
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
						, cameraNode->getCameraMatrix()
						, cameraNode->getDistortionCoefficients());
				}
			}

			// Draw on preview
			{
				// First layer is the undistorted difference image
				preview = differenceImage.clone();

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
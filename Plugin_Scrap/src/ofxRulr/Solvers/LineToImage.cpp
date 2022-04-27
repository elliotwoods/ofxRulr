#include "pch_Plugin_Scrap.h"
#include "LineToImage.h"
#include <opencv2/opencv.hpp>

const bool previewEnabled = false;

struct LineToPointsCost
{
	LineToPointsCost(const glm::vec2& point
		, const float& weight)
		: point(point)
		, weight(weight)
	{

	}

	template<typename T>
	bool
		operator()(const T* const parameters
			, T* residuals) const
	{
		// Construct line from parameters
		ofxRulr::Models::Line_<T> line;
		line.fromParameters(parameters);

		auto distance = line.distanceToPoint((glm::tvec2<T>) this->point);
		residuals[0] = (T) weight * distance;
		return true;
	}

	static ceres::CostFunction*
		Create(const glm::vec2& point
			, const float& weight)
	{
		return new ceres::AutoDiffCostFunction<LineToPointsCost, 1, 3>(
			new LineToPointsCost(point, weight)
			);
	}

	const glm::vec2& point;
	const float& weight;
};

namespace ofxRulr {
	namespace Solvers {
		//----------
		ofxCeres::SolverSettings
			LineToImage::defaultSolverSettings()
		{
			ofxCeres::SolverSettings solverSettings;
			solverSettings.options.function_tolerance = 0;
			solverSettings.options.parameter_tolerance= 0;
			solverSettings.options.max_num_iterations = 1000;
			return solverSettings;
		}

		//----------
		LineToImage::Result
			LineToImage::solve(const cv::Mat& image
				, uint8_t minimumValue
				, const ofxCeres::SolverSettings& solverSettings)
		{
			if (image.empty()) {
				throw(ofxRulr::Exception("Image is empty"));
			}
			if (image.channels() != 1) {
				throw(ofxRulr::Exception("Image channel count != 1"));
			}

			//Adaptive threshold
			cv::Mat blurred;
			cv::blur(image, blurred, cv::Size(64, 64));
			cv::Mat adaptiveThreshold;
			cv::addWeighted(image, 1, blurred, -1, 0, adaptiveThreshold);
			cv::Mat thresholded;
			cv::threshold(adaptiveThreshold, thresholded, minimumValue, 255, cv::THRESH_BINARY);
			cv::erode(thresholded, thresholded, cv::getStructuringElement(cv::MORPH_RECT, cv::Size(3, 3), cv::Point2i(2, 2)));

			// Accumulate the positive pixels from image
			vector<glm::vec2> points;
			vector<float> weights;
			imageToData(thresholded
				, minimumValue
				, points
				, weights);

			if (solverSettings.printReport) {
				cout << "Solving line with " << points.size() << " points" << endl;
			}

			// Initialise parameters
			double parameters[3];
			{
				// Take a first guess using OpenCV
				{
					vector<cv::Point2i> nonZeroPoints;
					for (const auto& point : points) {
						nonZeroPoints.emplace_back(ofxCv::toCv(point));
					}

					// tx, ty, x0, y0
					cv::Vec4f lineFirstGuessOpenCV;
					cv::fitLine(nonZeroPoints
						, lineFirstGuessOpenCV
						, cv::DIST_L2
						, 0
						, 1e-2
						, 1e-2);
					
					// push to parameters
					Models::Line lineFirstGuess(lineFirstGuessOpenCV);
					lineFirstGuess.toParameters(parameters);

					// make a preview
					if(previewEnabled) {
						auto preview = image.clone();
						lineFirstGuess.drawOnImage(preview);
						
						cv::pyrDown(preview, preview);
						cv::pyrDown(preview, preview);
						cv::pyrDown(preview, preview);

						cv::imshow("Preview", preview);
						cv::waitKey(0);
						cv::destroyAllWindows();
					}
				}
			}

			ceres::Problem problem;

			// Add data to problem
			{
				for (size_t i = 0; i < points.size(); i++) {
					problem.AddResidualBlock(LineToPointsCost::Create(points[i], weights[i])
						, NULL
						, parameters);
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
				result.solution.line.fromParameters(parameters);
				result.solution.meanResidual = getMeanResidual(points, weights, result.solution);

				// make a preview
				if (previewEnabled) {
					auto preview = image.clone();
					result.solution.line.drawOnImage(preview);

					cv::pyrDown(preview, preview);
					cv::pyrDown(preview, preview);
					cv::pyrDown(preview, preview);

					cv::imshow("Preview", preview);
					cv::waitKey(0);
					cv::destroyAllWindows();
				}

				return result;
			}
		}


		//----------
		float
			LineToImage::getResidual(const vector<glm::vec2>& points
				, const vector<float>& weights
				, const Solution& solution)
		{
			// Format solution for cost function
			float parameters[4];
			solution.line.toParameters(parameters);

			float totalResidual = 0.0f;
			for (size_t i = 0; i < points.size(); i++) {
				auto costFunction = LineToPointsCost(points[i], weights[i]);
				float residual;
				if (!costFunction(parameters, &residual)) {
					throw(ofxRulr::Exception("Failed to evaluate cost"));
				}

				totalResidual += residual;
			}
			return totalResidual;
		}

		//----------
		float
			LineToImage::getMeanResidual(const vector<glm::vec2>& points
				, const vector<float>& weights
				, const Solution& solution)
		{
			auto residual = LineToImage::getResidual(points, weights, solution);

			float totalWeight = 0.0f;
			for (const auto& weight : weights) {
				totalWeight += weight;
			}

			return residual / totalWeight;
		}

		//----------
		void
			LineToImage::imageToData(const cv::Mat& image
				, uint8_t minimumValue
				, vector<glm::vec2> & points
				, vector<float> & weights)
		{
			for (int j = 0; j < image.rows; j++) {
				for (int i = 0; i < image.cols; i++) {
					auto pixelValue = image.at<uint8_t>(j, i);
					if (pixelValue >= minimumValue) {
						points.push_back({ i, j });
						weights.push_back(pixelValue);
					}
				}
			}
		}
	}
}
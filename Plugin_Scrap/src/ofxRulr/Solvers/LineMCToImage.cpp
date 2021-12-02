#include "pch_Plugin_Scrap.h"
#include "LineMCToImage.h"
#include <opencv2/opencv.hpp>

const bool previewEnabled = true;

struct LineMCToPointsCost
{
	LineMCToPointsCost(const glm::vec2& point
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
		ofxRulr::Solvers::LineMCToImage::Line_<T> line;
		line.fromParameters(parameters);

		auto y_ = line.m * (T) point.x + line.c;
		residuals[0] = (T) weight * (y_ - (T) point.y);
		return true;
	}

	static ceres::CostFunction*
		Create(const glm::vec2& point
			, const float& weight)
	{
		return new ceres::AutoDiffCostFunction<LineMCToPointsCost, 1, 2>(
			new LineMCToPointsCost(point, weight)
			);
	}

	const glm::vec2& point;
	const float& weight;
};

namespace ofxRulr {
	namespace Solvers {
		//----------
		ofxCeres::SolverSettings
			LineMCToImage::defaultSolverSettings()
		{
			ofxCeres::SolverSettings solverSettings;
			solverSettings.options.function_tolerance = 0;
			solverSettings.options.parameter_tolerance= 0;
			solverSettings.options.max_num_iterations = 1000;
			return solverSettings;
		}

		//----------
		LineMCToImage::Result
			LineMCToImage::solve(const cv::Mat& image
				, uint8_t minimumValue
				, const ofxCeres::SolverSettings& solverSettings)
		{
			if (image.empty()) {
				throw(ofxRulr::Exception("Image is empty"));
			}
			if (image.channels() != 1) {
				throw(ofxRulr::Exception("Image channel count != 1"));
			}

			// Accumulate the positive pixels from image
			vector<glm::vec2> points;
			vector<float> weights;
			imageToData(image
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
					cv::Mat thresholded;
					auto threshold = minimumValue;
					if (minimumValue < 100) {
						threshold *= 2;
					}
					cv::threshold(image, thresholded, minimumValue, 255, cv::THRESH_BINARY);
					vector<cv::Point2i> nonZeroPoints;
					cv::findNonZero(thresholded, nonZeroPoints);

					// tx, ty, x0, y0
					cv::Vec4f lineFirstGuessOpenCV;
					cv::fitLine(nonZeroPoints
						, lineFirstGuessOpenCV
						, cv::DIST_L2
						, 0
						, 1e-2
						, 1e-2);
					
					// push to parameters
					Line lineFirstGuess(lineFirstGuessOpenCV);
					lineFirstGuess.toParameters(parameters);

					// make a preview
					if(previewEnabled) {
						auto preview = image.clone();
						lineFirstGuess.drawOnImage(preview);
						
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
					problem.AddResidualBlock(LineMCToPointsCost::Create(points[i], weights[i])
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

					for (int i = 0; i < points.size(); i++) {
						cv::circle(preview, ofxCv::toCv(points[i]), weights[i], cv::Scalar(255));
					}

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
			LineMCToImage::getResidual(const vector<glm::vec2>& points
				, const vector<float>& weights
				, const Solution& solution)
		{
			// Format solution for cost function
			float parameters[4];
			solution.line.toParameters(parameters);

			float totalResidual = 0.0f;
			for (size_t i = 0; i < points.size(); i++) {
				auto costFunction = LineMCToPointsCost(points[i], weights[i]);
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
			LineMCToImage::getMeanResidual(const vector<glm::vec2>& points
				, const vector<float>& weights
				, const Solution& solution)
		{
			auto residual = LineMCToImage::getResidual(points, weights, solution);

			float totalWeight = 0.0f;
			for (const auto& weight : weights) {
				totalWeight += weight;
			}

			return residual / totalWeight;
		}

		//----------
		void
			LineMCToImage::imageToData(const cv::Mat& image
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
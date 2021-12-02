#include "pch_Plugin_Scrap.h"
#include "PointFromLines.h"
#include <opencv2/opencv.hpp>

const bool previewEnabled = false;

typedef ofxRulr::Solvers::LineToImage::Line Line;

struct PointFromLinesCost
{
	PointFromLinesCost(const Line& line)
		: lineABC(line.getABC())
	{

	}

	template<typename T>
	bool
		operator()(const T* const pointParameters
			, T* residuals) const
	{
		auto lineABC = (glm::tvec3<T>)this->lineABC;
		auto point = glm::tvec2<T>(pointParameters[0], pointParameters[1]);
		residuals[0] = ofxCeres::VectorMath::distanceLineToPoint(lineABC, point);
		return true;
	}

	static ceres::CostFunction*
		Create(const Line& line)
	{
		return new ceres::AutoDiffCostFunction<PointFromLinesCost, 1, 2>(
			new PointFromLinesCost(line)
			);
	}

	const glm::vec3 lineABC;
};

namespace ofxRulr {
	namespace Solvers {
		//----------
		ofxCeres::SolverSettings
			PointFromLines::defaultSolverSettings()
		{
			ofxCeres::SolverSettings solverSettings;
			return solverSettings;
		}

		//----------
		PointFromLines::Result
			PointFromLines::solve(const vector<LineToImage::Line>& lines
				, const ofxCeres::SolverSettings& solverSettings)
		{
			if (lines.empty()) {
				throw(ofxRulr::Exception("No lines"));
			}

			// Initialise parameters
			glm::tvec2<double> point( 0.0, 0.0 );
			
			ceres::Problem problem;

			// Add data to problem
			for (const auto& line : lines) {
				problem.AddResidualBlock(PointFromLinesCost::Create(line)
					, NULL
					, &point[0]);
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
				result.solution.point = (glm::vec2)point;
				return result;
			}
		}
	}
}
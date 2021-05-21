#include "pch_Plugin_Experiments.h"
#include "HeliostatActionModel.h"

using namespace ofxCeres::VectorMath;

struct HeliostatActionModel_PositionCost
{
	HeliostatActionModel_PositionCost(const float& angle
		, const glm::vec3& polynomial)
		: angle(angle)
		, polynomial(polynomial)
	{

	}

	template<typename T>
	bool
		operator()(const T* const parameters
			, T* residuals) const
	{
		const auto & position = parameters[0];
		const auto polynomial = (glm::tvec3<T>) this->polynomial;
		auto angle = ofxRulr::Solvers::HeliostatActionModel::positionToAngle<T>(position, polynomial);

		auto delta = angle - (T)this->angle;
		residuals[0] = delta * delta;

		return true;
	}

	static ceres::CostFunction*
		Create(const float& angle
			, const glm::vec3& polynomial)
	{
		return new ceres::AutoDiffCostFunction<HeliostatActionModel_PositionCost, 1, 1>(
			new HeliostatActionModel_PositionCost(angle, polynomial)
			);
	}

	const float& angle;
	const glm::vec3& polynomial;
};

namespace ofxRulr {
	namespace Solvers {
		//----------
		ofxCeres::SolverSettings 
			HeliostatActionModel::SolvePosition::defaultSolverSettings()
		{
			ofxCeres::SolverSettings solverSettings;
			solverSettings.options.max_num_iterations = 100;
			solverSettings.options.parameter_tolerance = 1e-4;
			solverSettings.options.minimizer_progress_to_stdout = false;
			solverSettings.printReport = false;
			return solverSettings;
		}

		//----------
		HeliostatActionModel::SolvePosition::Result
			HeliostatActionModel::SolvePosition::solvePosition(const float& angle
				, const glm::vec3& polynomial
				, const Nodes::Experiments::MirrorPlaneCapture::Dispatcher::RegisterValue& priorPosition
				, const ofxCeres::SolverSettings& solverSettings)
		{
			// Initialise parameters
			double parameters[1];
			{
				parameters[0] = (double)priorPosition;
			}

			// Construct the problem
			ceres::Problem problem;
			{
				auto costFunction = HeliostatActionModel_PositionCost::Create(angle, polynomial);
				problem.AddResidualBlock(costFunction
					, NULL
					, parameters);
			}

			// Solve the fit
			ceres::Solver::Summary summary;
			ceres::Solve(solverSettings.options
				, &problem
				, &summary);

			if (solverSettings.printReport) {
				std::cout << summary.FullReport() << "\n";
			}

			// Build the result
			HeliostatActionModel::SolvePosition::Result result(summary);
			{
				result.solution = (int)parameters[0];
			}

			return result;
		}
	}
}
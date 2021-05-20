#include "pch_Plugin_Experiments.h"
#include "HeliostatActionModel.h"

using namespace ofxCeres::VectorMath;

struct HeliostatActionModel_AngleCost
{
	HeliostatActionModel_AngleCost(const int & registerValue
		, const glm::vec3& polynomial)
		: registerValue(registerValue)
		, polynomial(polynomial)
	{

	}

	template<typename T>
	bool
		operator()(const T* const parameters
			, T* residuals) const
	{
		const auto & value = parameters[0];
		const auto polynomial = (glm::tvec3<T>) this->polynomial;
		auto position = ofxRulr::Solvers::HeliostatActionModel::angleToPosition<T>(value, polynomial);

		auto delta = position - (T)this->registerValue;
		residuals[0] = delta * delta;

		return true;
	}

	static ceres::CostFunction*
		Create(const int& registerValue
			, const glm::vec3& polynomial)
	{
		return new ceres::AutoDiffCostFunction<HeliostatActionModel_AngleCost, 1, 1>(
			new HeliostatActionModel_AngleCost(registerValue, polynomial)
			);
	}

	const int& registerValue;
	const glm::vec3& polynomial;
};

namespace ofxRulr {
	namespace Solvers {
		//----------
		ofxCeres::SolverSettings 
			HeliostatActionModel::SolveAngle::defaultSolverSettings()
		{
			ofxCeres::SolverSettings solverSettings;
			solverSettings.options.max_num_iterations = 100;
			solverSettings.options.parameter_tolerance = 1e-4;
			solverSettings.options.minimizer_progress_to_stdout = false;
			solverSettings.printReport = false;
			return solverSettings;
		}

		//----------
		HeliostatActionModel::SolveAngle::Result
			HeliostatActionModel::SolveAngle::solveAngle(const Nodes::Experiments::MirrorPlaneCapture::Dispatcher::RegisterValue& registerValue
				, const glm::vec3& polynomial
				, const float& priorAngle
				, const ofxCeres::SolverSettings& solverSettings)
		{
			// Initialise parameters
			double parameters[1];
			{
				parameters[0] = (double)priorAngle;
			}

			// Construct the problem
			ceres::Problem problem;
			{
				auto costFunction = HeliostatActionModel_AngleCost::Create(registerValue, polynomial);
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
			HeliostatActionModel::SolveAngle::Result result(summary);
			{
				result.solution = (int)parameters[0];
			}

			return result;
		}
	}
}
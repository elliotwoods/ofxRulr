#include "pch_Plugin_Scrap.h"
#include "NavigateToWorldPoint.h"

const bool previewEnabled = false;

struct NavigateToWorldPointCost
{
	NavigateToWorldPointCost(const glm::vec3& point
		, const ofxRulr::Models::LaserProjector& laserProjector)
		: point(point)
		, laserProjector(laserProjector)
	{

	}

	template<typename T>
	bool
		operator()(const T* const parameters
			, T* residuals) const
	{
		auto laserProjector = this->laserProjector.castTo<T>();

		auto projectionPoint = *(glm::tvec2<T>*)(parameters);

		const auto ray = laserProjector.castRayWorldSpace(projectionPoint);
		const auto target = (glm::tvec3<T>) this->point;
		const auto closestPointOnRayToTarget = ray.closestPointOnRayTo(target);
		const auto delta = target - closestPointOnRayToTarget;
		residuals[0] = delta[0];
		residuals[1] = delta[1];
		residuals[2] = delta[2];
		return true;
	}

	static ceres::CostFunction*
		Create(const glm::vec3& point
			, const ofxRulr::Models::LaserProjector& laserProjector)
	{
		return new ceres::AutoDiffCostFunction<NavigateToWorldPointCost, 3, 2>(
			new NavigateToWorldPointCost(point, laserProjector)
			);
	}

	const glm::vec3& point;
	const ofxRulr::Models::LaserProjector& laserProjector;
};

namespace ofxRulr {
	namespace Solvers {
		//----------
		ofxCeres::SolverSettings
			NavigateToWorldPoint::defaultSolverSettings()
		{
			ofxCeres::SolverSettings solverSettings;
			solverSettings.options.function_tolerance = 1e-12;
			solverSettings.options.parameter_tolerance = 1e-8;
			solverSettings.options.max_num_iterations = 10000;
			solverSettings.printReport = false;
			solverSettings.options.minimizer_progress_to_stdout = false;
			solverSettings.options.logging_type = ceres::LoggingType::SILENT;
			return solverSettings;
		}

		//----------
		NavigateToWorldPoint::Result
			NavigateToWorldPoint::solve(const glm::vec3& worldPoint
				, const Models::LaserProjector& laserProjector
				, const glm::vec2& initialGuess
				, const ofxCeres::SolverSettings& solverSettings)
		{
			// Initialise parameters
			double parameters[2];
			{
				parameters[0] = (double)initialGuess[0];
				parameters[1] = (double) initialGuess[1];
			}

			ceres::Problem problem;

			// Add data to problem
			{
				problem.AddResidualBlock(NavigateToWorldPointCost::Create(worldPoint, laserProjector)
					, NULL
					, parameters);
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
				result.solution.point[0] = parameters[0];
				result.solution.point[1] = parameters[1];
				return result;
			}
		}
	}
}
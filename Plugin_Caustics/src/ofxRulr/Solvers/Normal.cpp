#include "pch_Plugin_Caustics.h"
#include "Normal.h"

template<typename T>
glm::tvec3<T> makeNormalFromParameters(const T* parameters)
{
	return glm::normalize(glm::tvec3<T>(parameters[0], parameters[1], (T)1.0));
}

struct NormalCost {
	//----------
	NormalCost(const glm::vec3 & incident
		, const glm::vec3 & refracted
		, float exitIORvsIncidentIOR)
		: incident(incident)
		, refracted(refracted)
		, exitIORvsIncidentIOR(exitIORvsIncidentIOR)
	{

	}

	//----------
	template<typename T>
	bool
		operator()(const T* const parameters
			, T* residuals) const
	{
		auto normal = makeNormalFromParameters(parameters);
		
		auto incident = (glm::tvec3<T>) this->incident;
		auto refractedDesired = (glm::tvec3<T>) this->refracted;
		auto eta = (T)this->exitIORvsIncidentIOR;
		
		auto refractedEstimated = ofxCeres::VectorMath::refract(incident
			, normal
			, eta);
		
		auto delta = refractedEstimated - refractedDesired;

		residuals[0] = delta[0];
		residuals[1] = delta[1];
		residuals[2] = delta[2];

		return true;
	}

	//----------
	static ceres::CostFunction*
		Create(const glm::vec3& incident
			, const glm::vec3& refracted
			, float exitIORvsIncidentIOR)
	{
		auto costFunction = new ceres::AutoDiffCostFunction<NormalCost, 3, 2>(
			new NormalCost(incident, refracted, exitIORvsIncidentIOR)
			);
		return costFunction;
	}

	const glm::vec3 incident;
	const glm::vec3 refracted;
	const float exitIORvsIncidentIOR;
};

namespace ofxRulr {
	namespace Solvers {
		//----------
		ofxCeres::SolverSettings
			Normal::getDefaultSolverSettings()
		{
			auto solverSettings = ofxCeres::SolverSettings();

			solverSettings.printReport = false;
			solverSettings.options.minimizer_type = ceres::MinimizerType::LINE_SEARCH;
			solverSettings.options.minimizer_progress_to_stdout = false;

			return solverSettings;
		}

		//----------
		Normal::Result
			Normal::solve(const glm::vec3& incident
				, const glm::vec3& refracted
				, float exitIORvsIncidentIOR
				, const ofxCeres::SolverSettings& solverSettings)
		{
			ceres::Problem problem;

			// Create parameters
			glm::tvec2<double> parameters{ 0.0, 0.0 };

			// Add the problem block
			problem.AddResidualBlock(NormalCost::Create(incident, refracted, exitIORvsIncidentIOR)
				, NULL
				, &parameters[0]);

			// Perform the solve
			ceres::Solver::Summary summary;
			{
				if (solverSettings.printReport) {
					cout << "Solve OpticalSystemSolver" << endl;
				}
				ceres::Solve(solverSettings.options
					, &problem
					, &summary);

				if (solverSettings.printReport) {
					cout << summary.FullReport() << endl;
				}
			}

			// Create the result
			{
				Result result(summary);
				result.solution = makeNormalFromParameters(&parameters[0]);
				return result;
			}
		}
	}
}
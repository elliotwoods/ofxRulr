#include "pch_Plugin_Experiments.h"
#include "HeliostatActionModel.h"

using namespace ofxCeres::VectorMath;

struct HeliostatActionModel_NormalCost
{
	HeliostatActionModel_NormalCost(const ofxRulr::Solvers::HeliostatActionModel::Parameters<float> & parameters
		, const glm::vec3& normal)
		: parameters(parameters)
		, normal(normal)
	{

	}

	template<typename T>
	bool
		operator()(const T* const parameters
			, T* residuals) const
	{
		auto hamParameters = this->parameters.castTo<T>();
		ofxRulr::Solvers::HeliostatActionModel::AxisAngles<T> axisAngles{
			parameters[0]
			, parameters[1]
		};

		auto mirrorPlane = ofxRulr::Solvers::HeliostatActionModel::getMirrorPlane(axisAngles
			, hamParameters);

		auto targetNormal = (glm::tvec3<T>)this->normal;
		auto dotProduct = ofxCeres::VectorMath::dot(mirrorPlane.normal, targetNormal);
		residuals[0] = acos(dotProduct);
		return true;
	}

	static ceres::CostFunction*
		Create(const ofxRulr::Solvers::HeliostatActionModel::Parameters<float>& parameters
			, const glm::vec3& normal)
	{
		return new ceres::AutoDiffCostFunction<HeliostatActionModel_NormalCost, 1, 2>(
			new HeliostatActionModel_NormalCost(parameters, normal)
			);
	}

	const ofxRulr::Solvers::HeliostatActionModel::Parameters<float> & parameters;
	const glm::vec3& normal;
};

namespace ofxRulr {
	namespace Solvers {
		//----------
		HeliostatActionModel::Navigator::Result
			HeliostatActionModel::Navigator::solveNormal(const HeliostatActionModel::Parameters<float>& hamParameters
				, const glm::vec3& normal
				, const HeliostatActionModel::AxisAngles<float>& initialAngles
				, const ofxCeres::SolverSettings& solverSettings)
		{
			// Initialise parameters
			double parameters[2];
			{
				parameters[0] = (double)initialAngles.axis1;
				parameters[1] = (double)initialAngles.axis2;
			}

			// Construct the problem
			ceres::Problem problem;
			{
				auto costFunction = HeliostatActionModel_NormalCost::Create(hamParameters, normal);
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
			HeliostatActionModel::Navigator::Result result(summary);
			{
				result.solution.axisAngles.axis1 = (float)parameters[0];
				result.solution.axisAngles.axis2 = (float)parameters[1];
			}

			return result;
		}
	}
}
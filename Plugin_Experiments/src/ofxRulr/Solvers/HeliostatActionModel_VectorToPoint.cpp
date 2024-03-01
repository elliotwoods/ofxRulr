#include "pch_Plugin_Experiments.h"
#include "HeliostatActionModel.h"

using namespace ofxCeres::VectorMath;

struct HeliostatActionModel_VectorToPointCost
{
	HeliostatActionModel_VectorToPointCost(const ofxRulr::Solvers::HeliostatActionModel::Parameters<float>& parameters
		, const glm::vec3& incidentVector
		, const glm::vec3& point)
		: parameters(parameters)
		, incidentVector(incidentVector)
		, point(point)
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

		ofxCeres::Models::Ray<T> incidentRay;
		{
			incidentRay.t = (glm::tvec3<T>) this->incidentVector;
			incidentRay.s = mirrorPlane.center - incidentRay.t;
		}
		auto reflectedRay = mirrorPlane.reflect(incidentRay);

		residuals[0] = reflectedRay.distanceTo((glm::tvec3<T>) this->point);

		// Residual for pointing wrong direction
		{
			auto intendedDirection = (glm::tvec3<T>) this->point - mirrorPlane.center;

			auto angleBetween = dot(normalize(intendedDirection), mirrorPlane.normal);
			if (angleBetween < (T)0) {
				residuals[1] = -angleBetween * (T)1000000.0;
			}
			else {
				residuals[1] = (T)0.0;
			}
		}

		return true;
	}

	static ceres::CostFunction*
		Create(const ofxRulr::Solvers::HeliostatActionModel::Parameters<float>& parameters
			, const glm::vec3& incidentVector
			, const glm::vec3& point)
	{
		return new ceres::AutoDiffCostFunction<HeliostatActionModel_VectorToPointCost, 2, 2>(
			new HeliostatActionModel_VectorToPointCost(parameters, incidentVector, point)
			);
	}

	const ofxRulr::Solvers::HeliostatActionModel::Parameters<float>& parameters;
	const glm::vec3& incidentVector;
	const glm::vec3& point;
};

namespace ofxRulr {
	namespace Solvers {
		//----------
		HeliostatActionModel::Navigator::Result
			HeliostatActionModel::Navigator::solveVectorToPoint(const HeliostatActionModel::Parameters<float>& hamParameters
				, const glm::vec3& incidentVector
				, const glm::vec3& point
				, const HeliostatActionModel::AxisAngles<float>& initialAngles
				, const ofxCeres::SolverSettings& solverSettings)
		{
			// First solve with a basic normal solve
			// (This is because we might get a reflection ray which is pointing directly away from target)
			auto normal = glm::normalize(-incidentVector) + glm::normalize(point - hamParameters.position);
			normal = glm::normalize(normal);
			auto initialSolution = Navigator::solveNormal(hamParameters
				, normal
				, initialAngles
				, solverSettings);

			// Initialise parameters
			double parameters[2];
			{
				parameters[0] = (double)initialSolution.solution.axisAngles.axis1;
				parameters[1] = (double)initialSolution.solution.axisAngles.axis2;
			}

			// Construct the problem
			ceres::Problem problem;
			{
				auto costFunction = HeliostatActionModel_VectorToPointCost::Create(hamParameters, incidentVector, point);
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
#include "pch_Plugin_Experiments.h"
#include "HeliostatActionModel.h"

using namespace ofxCeres::VectorMath;

struct HeliostatActionModel_PointToPointCost
{
	HeliostatActionModel_PointToPointCost(const ofxRulr::Solvers::HeliostatActionModel::Parameters<float>& parameters
		, const glm::vec3& pointA
		, const glm::vec3& pointB)
		: parameters(parameters)
		, pointA(pointA)
		, pointB(pointB)
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

		glm::tvec3<T> mirrorCenter, mirrorNormal;
		ofxRulr::Solvers::HeliostatActionModel::getMirrorCenterAndNormal(axisAngles
			, hamParameters
			, mirrorCenter
			, mirrorNormal);

		auto incidentTransmission = normalize(mirrorCenter - (glm::tvec3<T>) pointA);
		auto incidentTransmissionNormal = dot(incidentTransmission, mirrorNormal) * mirrorNormal;
		auto incidentTransmissionInPlane = incidentTransmission - incidentTransmissionNormal;

		auto reflectedTransmission = incidentTransmissionInPlane - incidentTransmissionNormal;

		residuals[0] = distanceRayToPoint(mirrorCenter
			, reflectedTransmission
			, (glm::tvec3<T>) this->pointB);


		// Residual for pointing wrong direction
		{
			auto intendedDirection = (glm::tvec3<T>) this->pointB - mirrorCenter;
			
			auto angleBetween = dot(normalize(intendedDirection), mirrorNormal);
			if (angleBetween < (T) 0) {
				//residuals[1] = -angleBetween * (T) 1000.0;
			}
			else {
				//residuals[1] = (T)0.0;
			}
		}

		return true;
	}

	static ceres::CostFunction*
		Create(const ofxRulr::Solvers::HeliostatActionModel::Parameters<float>& parameters
			, const glm::vec3& pointA
			, const glm::vec3& pointB)
	{
		return new ceres::AutoDiffCostFunction<HeliostatActionModel_PointToPointCost, 1, 2>(
			new HeliostatActionModel_PointToPointCost(parameters, pointA, pointB)
			);
	}

	const ofxRulr::Solvers::HeliostatActionModel::Parameters<float>& parameters;
	const glm::vec3& pointA;
	const glm::vec3& pointB;
};

namespace ofxRulr {
	namespace Solvers {
		//----------
		HeliostatActionModel::Navigator::Result
			HeliostatActionModel::Navigator::solvePointToPoint(const HeliostatActionModel::Parameters<float>& hamParameters
				, const glm::vec3& pointA
				, const glm::vec3& pointB
				, const HeliostatActionModel::AxisAngles<float>& initialAngles
				, const ofxCeres::SolverSettings& solverSettings)
		{
			// First solve with a basic normal solve
			// (This is because we might get a reflection ray which is pointing directly away from target)
			auto normal = glm::normalize(pointA - hamParameters.position) + glm::normalize(pointB - hamParameters.position);
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
				auto costFunction = HeliostatActionModel_PointToPointCost::Create(hamParameters, pointA, pointB);
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
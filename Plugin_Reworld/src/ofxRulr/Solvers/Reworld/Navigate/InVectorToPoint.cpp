#include "pch_Plugin_Reworld.h"
#include "InVectorToPoint.h"
#include "ofxCeres.h"

struct NavigateInVectorToPointCost
{
	NavigateInVectorToPointCost(const glm::vec3& inVector
		, const glm::vec3& point
		, const ofxRulr::Models::Reworld::Module<float>& module)
		: module(module)
		, point(point)
	{
		this->incomingRay.s = module.getPosition() - inVector;
		this->incomingRay.t = inVector;
	}

	template<typename T>
	bool
		operator()(const T* const axisAngleParameters
			, T* residuals) const
	{
		// Construct the axis angles
		ofxRulr::Models::Reworld::Module<T>::AxisAngles axisAngles;
		{
			axisAngles.A = axisAngleParameters[0];
			axisAngles.B = axisAngleParameters[1];
		}

		// Get the typed data
		auto typedModule = this->module.castTo<T>();
		auto typedIncomingRay = this->incomingRay.castTo<T>();
		auto typedTargetPoint = (glm::tvec3<T>) this->point;

		auto refractionResult = typedModule.refract(typedIncomingRay, axisAngles);
		auto closestPointOnRay = refractionResult.outputRay.closestPointOnRayTo(typedTargetPoint);
		auto delta = typedTargetPoint - closestPointOnRay;

		residuals[0] = delta[0];
		residuals[1] = delta[1];
		residuals[2] = delta[2];

		return true;
	}

	static ceres::CostFunction*
		Create(const glm::vec3& inVector
			, const glm::vec3& point
			, const ofxRulr::Models::Reworld::Module<float>& module)
	{
		return new ceres::AutoDiffCostFunction<NavigateInVectorToPointCost, 3, 2>(
			new NavigateInVectorToPointCost(inVector, point, module)
			);
	}

	const ofxRulr::Models::Reworld::Module<float> module;
	ofxCeres::Models::Ray<float> incomingRay;
	const glm::vec3 point;
};

namespace ofxRulr {
	namespace Solvers {
		namespace Reworld {
			namespace Navigate {
				//---------
				ofxCeres::SolverSettings
					InVectorToPoint::defaultSolverSettings()
				{
					ofxCeres::SolverSettings solverSettings;
					solverSettings.setPrintingEnabled(false);
					solverSettings.options.max_num_iterations = 1000;
					solverSettings.options.function_tolerance = 1e-6;

					return solverSettings;
				}

				//---------
				Result
					InVectorToPoint::solve(const Models::Reworld::Module<float>& module
						, const Models::Reworld::Module<float>::AxisAngles& initialAxisAngles
						, const glm::vec3& inVector
						, const glm::vec3& point
						, const ofxCeres::SolverSettings& solverSettings)
				{
					// Initial parameters
					double parameters[2];
					{
						parameters[0] = (double)initialAxisAngles.A;
						parameters[1] = (double)initialAxisAngles.B;
					}

					ceres::Problem problem;
					

					problem.AddResidualBlock(NavigateInVectorToPointCost::Create(inVector, point, module)
						, NULL
						, parameters);

					ceres::Solver::Summary summary;
					{
						ceres::Solve(solverSettings.options
							, &problem
							, &summary);

						if (solverSettings.printReport) {
							cout << summary.FullReport() << endl;
						}
					}

					{
						Result result(summary);
						result.solution.axisAngles.A = (float)parameters[0];
						result.solution.axisAngles.B = (float)parameters[1];
						return result;
					}

				}
			}
		}
	}
}


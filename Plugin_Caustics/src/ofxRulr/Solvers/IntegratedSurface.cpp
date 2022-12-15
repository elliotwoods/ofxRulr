#include "pch_Plugin_Caustics.h"
#include "IntegratedSurface.h"

struct IntegratedSurfaceCost
{
	//----------
	IntegratedSurfaceCost(const ofxRulr::Models::IntegratedSurface_<double>& priorSurface)
		: priorSurface(priorSurface)
	{

	}

	//----------
	template<typename T>
	bool
		operator()(const T* const * gridDistortionParameters
			, T* residuals) const
	{
		auto normalsSurface = this->priorSurface.castTo<T>();
		normalsSurface.distortedGrid.fromParameters(gridDistortionParameters[0]);
		normalsSurface.integrateHeights(residuals);
		return true;
	}

	//----------
	static ceres::CostFunction*
		Create(const ofxRulr::Models::IntegratedSurface_<double>& priorSurface)
	{
		auto costFunction = new ceres::DynamicAutoDiffCostFunction<IntegratedSurfaceCost, 4>(
			new IntegratedSurfaceCost(priorSurface)
			);
		costFunction->AddParameterBlock(priorSurface.distortedGrid.getParameterCount());
		costFunction->SetNumResiduals(priorSurface.getResidualCount());
		return costFunction;
	}

	ofxRulr::Models::IntegratedSurface_<double> priorSurface;
};

namespace ofxRulr {
	namespace Solvers {
		//----------
		ofxCeres::SolverSettings
			IntegratedSurface::getDefaultSolverSettings()
		{
			auto solverSettings = ofxCeres::SolverSettings();
			return solverSettings;
		}

		//----------
		IntegratedSurface::Result
			IntegratedSurface::solve(const Models::IntegratedSurface& initialCondition
				, const ofxCeres::SolverSettings& solverSettings)
		{
			if (initialCondition.distortedGrid.positions.empty()) {
				throw(ofxCeres::Exception("Grid is empty"));
			}

			ceres::Problem problem;

			// Create the surface
			auto integratedSurface = initialCondition.castTo<double>();

			// Create parameters
			vector<double*> allParameters;
			{
				// Grid parameters
				{
					const auto parametersCount = integratedSurface.distortedGrid.getParameterCount();
					auto parameters = new double[parametersCount];
					for (size_t i = 0; i < parametersCount; i++) {
						parameters[i] = 0.0;
					}
					allParameters.push_back(parameters);
				}
			}

			// Add the problem block
			problem.AddResidualBlock(IntegratedSurfaceCost::Create(integratedSurface)
				, NULL
				, allParameters);

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

			// Bring parameters back
			{
				integratedSurface.distortedGrid.fromParameters(allParameters[0]);
			}

			// Destroy parameter blocks
			{
				for (const auto& parameterBlock : allParameters) {
					delete[] parameterBlock;
				}
			}

			// Create the result
			{
				Result result(summary);
				result.solution.surface = integratedSurface.castTo<float>();
				return result;
			}
		}
	}
}
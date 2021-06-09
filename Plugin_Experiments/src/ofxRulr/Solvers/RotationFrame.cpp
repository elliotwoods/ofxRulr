#include "pch_Plugin_Experiments.h"
#include "RotationFrame.h"

using namespace ofxCeres::VectorMath;

template<typename T>
glm::tquat<T> rodriguesToQuat(const glm::tvec3<T> & rodrigues) {
	auto direction = glm::normalize(rodrigues);
	auto angle = glm::length(rodrigues);
	return glm::tquat<T>::rotate(angle, direction);
}

struct RotationFrameCost
{
	RotationFrameCost(const glm::vec3 & preRotation
		, const glm::vec3 & postRotation)
		: preRotation(preRotation)
		, postRotation(postRotation)
	{

	}
	template<typename T>
	bool
		operator()(const T* const parameters
			, T* residuals) const
	{
		glm::tvec3<T> rodrigues {
			parameters[0]
			, parameters[1]
			, parameters[2]
		};
		auto rotation = rodriguesToQuat(rodrigues);

		auto rotated = rotation * this->preRotation;
		residuals[0] = acos(glm::dot(glm::normalize(rotated), glm::normalize(postRotation)));

		return true;
	}

	static ceres::CostFunction*
		Create(const glm::vec3 & preRotation
		, const glm::vec3 & postRotation)
	{
		return new ceres::AutoDiffCostFunction<MirrorPlaneFromRaysCost, 1, 3>(
			new RotationFrameCost(preRotation, postRotation)
			);
	}

	const glm::vec3 & preRotation;
	const glm::vec3 & postRotation;
};

namespace ofxRulr {
	namespace Solvers {
		//----------
		ofxCeres::SolverSettings
			RotationFrame::defaultSolverSettings()
		{
			ofxCeres::SolverSettings solverSettings;
			return solverSettings;
		}

		//----------
		RotationFrame::Result
			RotationFrame::solve(const vector<glm::vec3> & preRotation
				, const vector<glm::vec3>& postRotation
				, const ofxCeres::SolverSettings& solverSettings)
		{
			// Check same length vectors
			if (preRotation.size() != postRotation.size()) {
				throw(ofxRulr::Exception("RotationFrame : input data length mismatch"));
			}
			if (preRotation.empty()) {
				throw(ofxRulr::Exception("RotationFrame : input data empty"));
			}

			// Initialise parameters
			double parameters[3];
			{
				parameters[0] = 0.0;
				parameters[1] = 0.0;
				parameters[2] = 0.0;
			}

			ceres::Problem problem;

			// Add all the correspondences
			{
				for (size_t i = 0; i < preRotation.size(); i++) {
					auto costFunction = RotationFrame::Create(preRotation[i], postRotation[i]);
					problem.AddResidualBlock(costFunction
						, NULL
						, parameters);
				}
			}

			ceres::Solver::Summary summary;

			ceres::Solve(solverSettings.options
				, &problem
				, &summary);

			if (solverSettings.printReport) {
				std::cout << summary.FullReport() << "\n";
			}

			{
				Result result(summary);
				result.solution.rotation = rodriguesToQuat<float>({
					parameters[0]
					, parameters[1]
					, parameters[2]
				});
				return result;
			}
		}
	}
}
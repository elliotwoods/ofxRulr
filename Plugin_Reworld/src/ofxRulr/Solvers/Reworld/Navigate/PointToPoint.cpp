#include "pch_Plugin_Reworld.h"
#include "PointToPoint.h"
#include "InVectorToPoint.h"

namespace ofxRulr {
	namespace Solvers {
		namespace Reworld {
			namespace Navigate {
				//---------
				ofxCeres::SolverSettings
					PointToPoint::defaultSolverSettings()
				{
					return InVectorToPoint::defaultSolverSettings();
				}

				//---------
				Result
					PointToPoint::solve(const Models::Reworld::Module<float> &module
						, const Models::Reworld::AxisAngles<float>& initialAxisAngles
						, const glm::vec3& point1
						, const glm::vec3& point2
						, const ofxCeres::SolverSettings & solverSettings)
				{
					auto modulePosition = module.getPosition();

					// We just use the other solver
					auto inVector = glm::normalize(modulePosition - point1);

					return InVectorToPoint::solve(module
						, initialAxisAngles
						, inVector
						, point2
						, solverSettings);
				}
			}
		}
	}
}
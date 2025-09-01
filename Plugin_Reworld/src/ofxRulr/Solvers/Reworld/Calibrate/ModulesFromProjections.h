#pragma once

#include "ofxRulr/Models/Reworld/Module.h"

namespace ofxRulr {
	namespace Solvers {
		namespace Reworld {
			namespace Calibrate {
				class ModuleFromProjections {
				public:
					struct Solution {
						glm::vec3 lightPosition;
						float interPrismDistance;
						float prismAngleRadians;
						float ior;
						vector<Models::Reworld::Module<float>> modules;
					};

					typedef ofxCeres::Result<Solution> Result;

					class Problem {
					public:
						Problem(const Solution& initialSolution
							, const vector<glm::vec3>& targetPositions);
						~Problem();

						void addProjectionObservation(int moduleIdx
							, int targetIdx
							, const Models::Reworld::AxisAngles<float>& axisAngles);

						// Globals
						void setLightPositionFixed();
						void setLightPositionVariable();

						void setInterPrismDistanceFixed();
						void setInterPrismDistanceVariable();

						void setPrismAngleFixed();
						void setPrismAngleVariable();

						void setIORFixed();
						void setIORVariable();

						// Per module
						void setAllModulePositionsFixed();
						void setAllModulePositionsVariable();

						void setAllModuleRotationsFixed();
						void setAllModuleRotationsVariable();

						void setAllAxisAngleOffsetsFixed();
						void setAllAxisAngleOffsetsVariable();

						Result solve(const ofxCeres::SolverSettings&);
					protected:
						ceres::Problem problem;

						double* lightPositionParameters;
						double* interPrismDistanceParameter;
						double* prismAngleParameter;
						double* iorParameter;

						vector<glm::tvec3<double>> targetPositions;

						vector<glm::tmat4x4<double>> moduleBulkTransforms;
						vector<double*> moduleTranslationParameters;
						vector<double*> moduleRotationParameters;
						vector<double*> moduleAxisAngleOffsetParameters;

					};

					static float getResidual(const Solution&
						, const Models::Reworld::Module<float>& module
						, const glm::vec3& lightPosition
						, const Models::Reworld::AxisAngles<float>& axisAngles);
				};
			}
		}
	}
}
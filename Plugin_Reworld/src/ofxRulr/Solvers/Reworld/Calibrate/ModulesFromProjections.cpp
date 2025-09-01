#include "pch_Plugin_Reworld.h"
#include "ModulesFromProjections.h"

struct ProjectionFromModuleCost {
	//----------
	ProjectionFromModuleCost(const glm::mat4x4& bulkTransform
		, const glm::vec3 targetPosition
		, const ofxRulr::Models::Reworld::AxisAngles<float>& axisAngles)
	{
		this->bulkTransform = (glm::tmat4x4<double>) bulkTransform;
		this->targetPosition = (glm::tvec3<double>)targetPosition;
		this->axisAngles[0] = (double)axisAngles.A;
		this->axisAngles[1] = (double)axisAngles.B;
	}

	//----------
	template<typename T>
	bool
		operator()(const T*  lightPositionParameters
			, const T* const interPrismDistanceParameter
			, const T* const prismAngleParameter
			, const T* const iorParameter
			, const T* const moduleTranslationParameters
			, const T* const moduleRotationParameters
			, const T* const moduleAxisAngleOffsetParameters
			, T* residuals) const
	{
		glm::tvec3<T> lightPosition;
		{
			lightPosition[0] = lightPositionParameters[0];
			lightPosition[1] = lightPositionParameters[1];
			lightPosition[2] = lightPositionParameters[2];
		}

		ofxRulr::Models::Reworld::Module<T> module;
		{
			module.bulkTransform = (glm::tmat4x4<T>) this->bulkTransform;
			
			// global parameters
			{
				module.installationParameters.interPrismDistance = interPrismDistanceParameter[0];
				module.installationParameters.prismAngleRadians = prismAngleParameter[0];
				module.installationParameters.ior = iorParameter[0];
			}

			// per-module parameters
			{
				for (int i = 0; i < 3; i++) {
					module.transformOffset.translation[i] = moduleTranslationParameters[i];
					module.transformOffset.rotationVector[i] = moduleRotationParameters[i];
				}

				module.axisAngleOffsets.A = moduleAxisAngleOffsetParameters[0];
				module.axisAngleOffsets.B = moduleAxisAngleOffsetParameters[1];
			}
		}

		ofxRulr::Models::Reworld::AxisAngles<T> axisAngles;
		{
			axisAngles.A = (T)this->axisAngles[0];
			axisAngles.B = (T)this->axisAngles[1];
		}

		auto targetPosition = (glm::tvec3<T>) this->targetPosition;

		auto modulePosition = module.getPosition();

		// Create the ray that travels from the light to the module
		ofxCeres::Models::Ray<T> incomingRay;
		{
			incomingRay.s = lightPosition;
			incomingRay.t = ofxCeres::VectorMath::normalize(modulePosition - lightPosition);
		}

		// Refract the ray through the module
		auto refractionResult = module.refract(incomingRay, axisAngles);

		// Compute residuals: if invalid, output zeros so this datapoint is ignored
		glm::tvec3<T> delta;
		bool ok = true;

		// Validate the refracted ray is finite before using it
		if (!(ceres::IsFinite(refractionResult.outputRay.s[0]) &&
			ceres::IsFinite(refractionResult.outputRay.s[1]) &&
			ceres::IsFinite(refractionResult.outputRay.s[2]) &&
			ceres::IsFinite(refractionResult.outputRay.t[0]) &&
			ceres::IsFinite(refractionResult.outputRay.t[1]) &&
			ceres::IsFinite(refractionResult.outputRay.t[2]))) {
			ok = false;
		}

		if (ok) {
			// Only proceed if the ray is valid
			auto closestPointOnRay = refractionResult.outputRay.closestPointOnRayTo(targetPosition);
			delta = targetPosition - closestPointOnRay;

			if (!(ceres::IsFinite(delta[0]) &&
				ceres::IsFinite(delta[1]) &&
				ceres::IsFinite(delta[2]))) {
				ok = false;
			}
		}

		// If invalid -> zero residuals (no influence). If valid -> use delta.
		if (!ok) {
			residuals[0] = T(0);
			residuals[1] = T(0);
			residuals[2] = T(0);
			return true;   // IMPORTANT: keep returning true
		}

		residuals[0] = delta[0];
		residuals[1] = delta[1];
		residuals[2] = delta[2];
		return true;

	}

	static ceres::CostFunction*
		Create(const glm::mat4x4& bulkTransform
			, const glm::vec3& targetPosition
			, const ofxRulr::Models::Reworld::AxisAngles<float>& axisAngles)
	{
		return new ceres::AutoDiffCostFunction<ProjectionFromModuleCost, 3, 3, 1, 1, 1, 3, 3, 2>(
			new ProjectionFromModuleCost(bulkTransform, targetPosition, axisAngles)
		);
	}

	glm::tmat4x4<double> bulkTransform;
	glm::tvec3<double> targetPosition;
	double axisAngles[2];
};

namespace ofxRulr {
	namespace Solvers {
		namespace Reworld {
			namespace Calibrate {
				//----------
				ModuleFromProjections::Problem::Problem(const Solution& initialSolution
					, const vector<glm::vec3>& targetPositions)
				{
					// Global parameters
					{
						this->lightPositionParameters = new double[3];
						for (int i = 0; i < 3; i++) {
							this->lightPositionParameters[i] = initialSolution.lightPosition[i];
						}

						this->interPrismDistanceParameter = new double[1];
						this->interPrismDistanceParameter[0] = initialSolution.interPrismDistance;

						this->prismAngleParameter = new double[1];
						this->prismAngleParameter[0] = initialSolution.prismAngleRadians;

						this->iorParameter = new double[1];
						this->iorParameter[0] = initialSolution.ior;
					}

					// Target positions
					{
						for (const auto& targetPosition : targetPositions) {
							this->targetPositions.push_back((glm::tvec3<double>)targetPosition);
						}
					}

					// Per module parameters
					{
						for (const auto& module : initialSolution.modules) {
							this->moduleBulkTransforms.push_back((glm::tmat4x4<double>) module.bulkTransform);

							auto translationParameters = new double[3];
							auto rotationParameters = new double[3];

							for (int i = 0; i < 3; i++) {
								translationParameters[i] = module.transformOffset.translation[i];
								rotationParameters[i] = module.transformOffset.rotationVector[i];
							}

							auto axisAngleOffsetParameters = new double[2];
							{
								axisAngleOffsetParameters[0] = module.axisAngleOffsets.A;
								axisAngleOffsetParameters[1] = module.axisAngleOffsets.B;
							}

							this->moduleTranslationParameters.push_back(translationParameters);
							this->moduleRotationParameters.push_back(rotationParameters);
							this->moduleAxisAngleOffsetParameters.push_back(axisAngleOffsetParameters);
						}
					}
				}

				//----------
				ModuleFromProjections::Problem::~Problem()
				{
					delete[] this->lightPositionParameters;
					delete[] this->interPrismDistanceParameter;
					delete[] this->prismAngleParameter;
					delete[] this->iorParameter;

					for (auto it : this->moduleTranslationParameters) {
						delete[] it;
					}
					for (auto it : this->moduleRotationParameters) {
						delete[] it;
					}
					for (auto it : this->moduleAxisAngleOffsetParameters) {
						delete[] it;
					}
				}

				//----------
				void
					ModuleFromProjections::Problem::addProjectionObservation(int moduleIdx
						, int targetIdx
						, const Models::Reworld::AxisAngles<float>& axisAngles)
				{
					if (moduleIdx >= this->moduleBulkTransforms.size()) {
						throw(ofxRulr::Exception("moduleIdx out of range"));
					}

					if (targetIdx >= this->targetPositions.size()) {
						throw(ofxRulr::Exception("targetIdx out of range"));
					}

					auto residualBlock = ProjectionFromModuleCost::Create(this->moduleBulkTransforms[moduleIdx]
						, this->targetPositions[targetIdx]
						, axisAngles);

					this->problem.AddResidualBlock(residualBlock
						, NULL
						, this->lightPositionParameters
						, this->interPrismDistanceParameter
						, this->prismAngleParameter
						, this->iorParameter
						, this->moduleTranslationParameters[moduleIdx]
						, this->moduleRotationParameters[moduleIdx]
						, this->moduleAxisAngleOffsetParameters[moduleIdx]);
				}

				//----------
				void
					ModuleFromProjections::Problem::setLightPositionFixed()
				{
					this->problem.SetParameterBlockConstant(this->lightPositionParameters);
				}

				//----------
				void
					ModuleFromProjections::Problem::setLightPositionVariable()
				{
					this->problem.SetParameterBlockVariable(this->lightPositionParameters);
				}

				//----------
				void
					ModuleFromProjections::Problem::setInterPrismDistanceFixed()
				{
					this->problem.SetParameterBlockConstant(this->interPrismDistanceParameter);
				}

				//----------
				void
					ModuleFromProjections::Problem::setInterPrismDistanceVariable()
				{
					this->problem.SetParameterBlockVariable(this->interPrismDistanceParameter);
				}

				//----------
				void
					ModuleFromProjections::Problem::setPrismAngleFixed()
				{
					this->problem.SetParameterBlockConstant(this->prismAngleParameter);
				}

				//----------
				void
					ModuleFromProjections::Problem::setPrismAngleVariable()
				{
					this->problem.SetParameterBlockVariable(this->prismAngleParameter);
				}

				//----------
				void
					ModuleFromProjections::Problem::setIORFixed()
				{
					this->problem.SetParameterBlockConstant(this->iorParameter);
				}

				//----------
				void
					ModuleFromProjections::Problem::setIORVariable()
				{
					this->problem.SetParameterBlockVariable(this->iorParameter);
				}

				//----------
				void
					ModuleFromProjections::Problem::setAllModulePositionsFixed()
				{
					for (auto* parameter : this->moduleTranslationParameters) {
						this->problem.SetParameterBlockConstant(parameter);
					}
				}

				//----------
				void
					ModuleFromProjections::Problem::setAllModulePositionsVariable()
				{
					for (auto* parameter : this->moduleTranslationParameters) {
						this->problem.SetParameterBlockVariable(parameter);
					}
				}

				//----------
				void
					ModuleFromProjections::Problem::setAllModuleRotationsFixed()
				{
					for (auto* parameter : this->moduleRotationParameters) {
						this->problem.SetParameterBlockConstant(parameter);
					}
				}

				//----------
				void
					ModuleFromProjections::Problem::setAllModuleRotationsVariable()
				{
					for (auto* parameter : this->moduleRotationParameters) {
						this->problem.SetParameterBlockVariable(parameter);
					}
				}

				//----------
				void
					ModuleFromProjections::Problem::setAllAxisAngleOffsetsFixed()
				{
					for (auto* parameter : this->moduleAxisAngleOffsetParameters) {
						this->problem.SetParameterBlockConstant(parameter);
					}
				}

				//----------
				void
					ModuleFromProjections::Problem::setAllAxisAngleOffsetsVariable()
				{
					for (auto* parameter : this->moduleAxisAngleOffsetParameters) {
						this->problem.SetParameterBlockVariable(parameter);
					}
				}

				//----------
				ModuleFromProjections::Result
					ModuleFromProjections::Problem::solve(const ofxCeres::SolverSettings& solverSettings)
				{
					// Perform solve
					ceres::Solver::Summary summary;
					{
						if (solverSettings.printReport) {
							cout << "Solve ModuleFromProjections" << endl;
						}

						ceres::Solve(solverSettings.options
							, &this->problem
							, &summary);

						if (solverSettings.printReport) {
							cout << summary.FullReport() << endl;
						}
					}

					// Read out result
					{
						Result result(summary);

						// Globals
						{
							result.solution.lightPosition = (glm::vec3)glm::tvec3<double>(
								this->lightPositionParameters[0]
								, this->lightPositionParameters[1]
								, this->lightPositionParameters[2]
							);

							result.solution.interPrismDistance = (float)this->interPrismDistanceParameter[0];
							result.solution.prismAngleRadians = (float)this->prismAngleParameter[0];
							result.solution.ior = (float)this->iorParameter[0];
						}

						// Per-module
						{
							const auto moduleCount = this->moduleTranslationParameters.size();
							result.solution.modules.reserve(moduleCount);

							for (size_t i = 0; i < moduleCount; i++) {
								Models::Reworld::Module<float> module;

								// Bulk transform is fixed per module in this problem
								module.bulkTransform = (glm::mat4)this->moduleBulkTransforms[i];

								// Translation / rotation (from parameter blocks)
								{
									auto* t = this->moduleTranslationParameters[i];
									auto* r = this->moduleRotationParameters[i];

									for (int c = 0; c < 3; c++) {
										module.transformOffset.translation[c] = (float)t[c];
										module.transformOffset.rotationVector[c] = (float)r[c];
									}
								}

								// Axis-angle offsets (A,B)
								{
									auto* a = this->moduleAxisAngleOffsetParameters[i];
									module.axisAngleOffsets.A = (float)a[0];
									module.axisAngleOffsets.B = (float)a[1];
								}

								result.solution.modules.push_back(module);
							}
						}

						return result;
					}

				}

				//----------
				float
					ModuleFromProjections::getResidual(const Solution& solution
						, const Models::Reworld::Module<float>& module
						, const glm::vec3& targetPosition
						, const Models::Reworld::AxisAngles<float>& axisAngles)
				{
					// Parameter blocks (stack-local), matching ProjectionFromModuleCost::operator()
					double lightPositionParameters[3];
					double interPrismDistanceParameter[1];
					double prismAngleParameter[1];
					double iorParameter[1];
					double moduleTranslationParameters[3];
					double moduleRotationParameters[3];
					double moduleAxisAngleOffsetParameters[2];

					// Fill globals
					{
						lightPositionParameters[0] = (double)solution.lightPosition.x;
						lightPositionParameters[1] = (double)solution.lightPosition.y;
						lightPositionParameters[2] = (double)solution.lightPosition.z;

						interPrismDistanceParameter[0] = (double)solution.interPrismDistance;
						prismAngleParameter[0] = (double)solution.prismAngleRadians;
						iorParameter[0] = (double)solution.ior;
					}

					// Fill per-module
					{
						for (int c = 0; c < 3; c++) {
							moduleTranslationParameters[c] = (double)module.transformOffset.translation[c];
							moduleRotationParameters[c] = (double)module.transformOffset.rotationVector[c];
						}
						moduleAxisAngleOffsetParameters[0] = (double)module.axisAngleOffsets.A;
						moduleAxisAngleOffsetParameters[1] = (double)module.axisAngleOffsets.B;
					}

					// Cost functor (not the ceres::CostFunction wrapper)
					ProjectionFromModuleCost costFunctor(
						(glm::mat4)module.bulkTransform
						, targetPosition
						, axisAngles);

					// Evaluate residuals
					double residuals[3];
					if (!costFunctor(lightPositionParameters
						, interPrismDistanceParameter
						, prismAngleParameter
						, iorParameter
						, moduleTranslationParameters
						, moduleRotationParameters
						, moduleAxisAngleOffsetParameters
						, residuals)) {
						throw(ofxRulr::Exception("ModuleFromProjections::getResidual : Failed to evaluate cost function"));
					}

					// RMS over 3D residual
					const double r0 = residuals[0];
					const double r1 = residuals[1];
					const double r2 = residuals[2];
					const float rms = (float)sqrt((r0 * r0 + r1 * r1 + r2 * r2) / 3.0);

					return rms;
				}

			}
		}
	}
}
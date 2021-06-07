#include "pch_Plugin_Experiments.h"
#include "HeliostatActionModel.h"


namespace ofxRulr {
	namespace Solvers {
#pragma mark Navigator
		//----------
		ofxCeres::SolverSettings
			HeliostatActionModel::Navigator::defaultSolverSettings()
		{
			ofxCeres::SolverSettings solverSettings;
			solverSettings.options.max_num_iterations = 5000;
			solverSettings.options.parameter_tolerance = 1e-4 * 360.0f / 4096.0f;
			return solverSettings;
		}

		//----------
		bool
			HeliostatActionModel::Navigator::validate(const Parameters<float>& hamParameters
				, const AxisAngles<float>& axisAngles)
		{
			return axisAngles.axis1 >= hamParameters.axis1.angleRange.minimum
				&& axisAngles.axis1 <= hamParameters.axis1.angleRange.maximum
				&& axisAngles.axis2 >= hamParameters.axis2.angleRange.minimum
				&& axisAngles.axis2 <= hamParameters.axis2.angleRange.maximum;
		}

		//----------
		bool
			HeliostatActionModel::Navigator::constrainAngles(const Parameters<float>& hamParameters
				, AxisAngles<float>& axisAngles
				, const AxisAngles<float>& initialAngles)
		{
			bool changed = false;
			
			// keep a copy in case the validation fails at the end
			auto unchangedVersion = axisAngles;

			if (hamParameters.axis1.angleRange.maximum - hamParameters.axis1.angleRange.minimum < 180.0f) {
				throw(ofxRulr::Exception("Axis 1 angle range is < 180 degrees"));
			}

			// Spin axis 1 into range
			{
				while (axisAngles.axis1 < hamParameters.axis1.angleRange.minimum) {
					axisAngles.axis1 += 180.0f;
					axisAngles.axis2 *= -1.0f;
					changed = true;
				}

				while (axisAngles.axis1 > hamParameters.axis1.angleRange.maximum) {
					axisAngles.axis1 -= 180.0f;
					axisAngles.axis2 *= -1.0f;
					changed = true;
				}
			}

			// Check if we've flipped axis 1 by > 90 degrees. And if so, move it back if it's valid to do so
			{
				const auto axis1Movement = axisAngles.axis1 - initialAngles.axis1;
				if (axis1Movement > 90) {
					auto flippedAngles = axisAngles;
					flippedAngles.axis1 -= 180.0f;
					flippedAngles.axis2 *= -1.0f;
					if (validate(hamParameters, flippedAngles)) {
						axisAngles = flippedAngles;
						changed = true;
					}
				}
				if (axis1Movement < -90) {
					auto flippedAngles = axisAngles;
					flippedAngles.axis1 += 180.0f;
					flippedAngles.axis2 *= -1.0f;
					if (validate(hamParameters, flippedAngles)) {
						axisAngles = flippedAngles;
						changed = true;
					}
				}
			}

			// Return the changed values if they are valid
			if (changed) {
				if (validate(hamParameters, axisAngles)) {
					return true;
				}
				else {
					axisAngles = unchangedVersion;
					return false;
				}
			}
			else {
				return false;
			}
		}

		HeliostatActionModel::Navigator::Result
			HeliostatActionModel::Navigator::solveConstrained(const Parameters<float>& hamParameters
				, std::function<Result(const AxisAngles<float>&)> solveFunction
				, const AxisAngles<float>& initialAngles
				, bool throwIfOutsideConstraints)
		{
			// Perform first solve
			auto result = solveFunction(initialAngles);

			// Constrain the angles to our range
			auto anglesNeededConstrain = Navigator::constrainAngles(hamParameters
				, result.solution.axisAngles
				, initialAngles);

			// If the angles were altered by constraints, then re-solve
			if (anglesNeededConstrain) {
				// Then solve again
				result = solveFunction(result.solution.axisAngles);
			}

			// Final check if we need to constrain angles
			anglesNeededConstrain = Navigator::constrainAngles(hamParameters
				, result.solution.axisAngles
				, initialAngles);
			if (anglesNeededConstrain) {
				result.isError = true;
				result.errorMessage = "Could not solve within constraints";
			}

			if (result.isError && throwIfOutsideConstraints) {
				throw(ofxRulr::Exception("Could not navigate mirror within bounds : " + result.errorMessage));
			}

			return result;
		}

		void
			HeliostatActionModel::drawMirror(const glm::vec3& mirrorCenter
				, const glm::vec3& mirrorNormal
				, float diameter)
		{
			auto up = (mirrorNormal == glm::vec3(0, -1, 0)
				|| mirrorNormal == glm::vec3(0, 1, 0))
				? glm::vec3(0, 0, 1)
				: glm::vec3(0, -1, 0);

			auto lookAt = glm::lookAt(mirrorCenter, mirrorCenter + mirrorNormal, up);

			ofPushMatrix();
			{
				ofMultMatrix(glm::inverse(lookAt));
				ofDrawCircle(glm::vec2(0, 0), diameter / 2.0f);
			}
			ofPopMatrix();
		}
	}
}
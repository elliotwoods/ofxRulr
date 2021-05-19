#pragma once
#include "ofxCeres.h"
#include <glm/glm.hpp>

namespace ofxRulr {
	namespace Solvers {
		class HeliostatActionModel {
		public:
			template<typename T>
			struct Parameters {
				struct Axis {
					glm::tvec3<T> polynomial;
					glm::tvec3<T> rotationAxis;
				};

				glm::tvec3<T> position;

				Axis axis1;
				Axis axis2;

				T mirrorOffset;

				template<typename T2>
				Parameters<T2> castTo() const {
					Parameters<T2> newParameters;
					newParameters.position = glm::tvec3<T2>(this->position);
					newParameters.axis1.polynomial = glm::tvec3<T2>(this->axis1.polynomial);
					newParameters.axis1.rotationAxis = glm::tvec3<T2>(this->axis1.rotationAxis);
					newParameters.axis2.polynomial = glm::tvec3<T2>(this->axis2.polynomial);
					newParameters.axis2.rotationAxis = glm::tvec3<T2>(this->axis2.rotationAxis);
					newParameters.mirrorOffset = (T2)this->mirrorOffset;
					return newParameters;
				}
			};

			template<typename T>
			struct AxisAngles {
				T axis1;
				T axis2;
			};

			template<typename T>
			static void getMirrorCenterAndNormal(const AxisAngles<T>& axisAngles
				, const Parameters<T>& parameters
				, glm::tvec3<T>& center
				, glm::tvec3<T>& normal) {

				auto mirrorTransform = glm::translate<T>(parameters.position)
					* glm::rotate<T>(axisAngles.axis1 * DEG_TO_RAD
						, parameters.axis1.rotationAxis)
					* glm::rotate<T>(axisAngles.axis2 * DEG_TO_RAD
						, parameters.axis2.rotationAxis)
					* glm::translate<T>(glm::tvec3<T>(0, -parameters.mirrorOffset, 0));

				center = ofxCeres::VectorMath::applyTransform<T>(mirrorTransform, { 0, 0, 0 });
				auto centerPlusNormal = ofxCeres::VectorMath::applyTransform<T>(mirrorTransform, { 0, -1, 0 });
				normal = ofxCeres::VectorMath::normalize(centerPlusNormal - center);
			}

			class Navigator {
			public:
				struct Solution {
					AxisAngles<float> axisAngles;
				};
				typedef ofxCeres::Result<Solution> Result;

				static ofxCeres::SolverSettings defaultSolverSettings();

				static Result solveNormal(const Parameters<float> &
					, const glm::vec3 & normal
					, const AxisAngles<float> & initialAngles
					, const ofxCeres::SolverSettings& solverSettings = defaultSolverSettings());

				static Result solvePointToPoint(const Parameters<float>&
					, const glm::vec3& pointA
					, const glm::vec3& pointB
					, const AxisAngles<float>& initialAngles
					, const ofxCeres::SolverSettings& solverSettings = defaultSolverSettings());
			};
		};
	}
}
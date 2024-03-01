#pragma once
#include "ofxCeres.h"
#include <glm/glm.hpp>

#include "ofxRulr/Nodes/Experiments/MirrorPlaneCapture/Dispatcher.h"
#include "ofxCeres/Models/Plane.h"

namespace ofxRulr {
	namespace Solvers {
		class HeliostatActionModel {
		public:
			template<typename T>
			struct Parameters {
				struct Axis {
					glm::tvec3<T> rotationAxis;
					glm::tvec3<T> polynomial;

					// Not that this is not used during calibration
					// It should be ignored when returned from calibration also
					struct {
						T minimum;
						T maximum;
					} angleRange;
				};

				glm::tvec3<T> position;
				T rotationY;

				Axis axis1;
				Axis axis2;

				T mirrorOffset;

				template<typename T2>
				Parameters<T2> castTo() const {
					Parameters<T2> newParameters;
					newParameters.position = glm::tvec3<T2>(this->position);
					newParameters.rotationY = (T2) this->rotationY;
					newParameters.axis1.polynomial = glm::tvec3<T2>(this->axis1.polynomial);
					newParameters.axis1.rotationAxis = glm::tvec3<T2>(this->axis1.rotationAxis);
					newParameters.axis1.angleRange.minimum = (T2)this->axis1.angleRange.minimum;
					newParameters.axis1.angleRange.maximum = (T2)this->axis1.angleRange.maximum;
					newParameters.axis2.polynomial = glm::tvec3<T2>(this->axis2.polynomial);
					newParameters.axis2.rotationAxis = glm::tvec3<T2>(this->axis2.rotationAxis);
					newParameters.axis2.angleRange.minimum = (T2)this->axis2.angleRange.minimum;
					newParameters.axis2.angleRange.maximum = (T2)this->axis2.angleRange.maximum;
					newParameters.mirrorOffset = (T2)this->mirrorOffset;
					return newParameters;
				}

				void toParameterStrings(T* positionParameters
					, T* rotationYParameters
					, T* rotationAxisParameters
					, T* polynomialParameters
					, T* mirrorOffsetParameters) const {
					// positionParameters
					positionParameters[0] = this->position[0];
					positionParameters[1] = this->position[1];
					positionParameters[2] = this->position[2];

					// rotation Y
					rotationYParameters[0] = this->rotationY;

					// rotationAxisParameters
					{
						{
							auto polar = ofxCeres::VectorMath::cartesianToPolar(this->axis1.rotationAxis);
							rotationAxisParameters[0] = polar[1];
							rotationAxisParameters[1] = polar[2];
						}
						{
							auto polar = ofxCeres::VectorMath::cartesianToPolar(this->axis2.rotationAxis);
							rotationAxisParameters[2] = polar[1];
							rotationAxisParameters[3] = polar[2];
						}
					}

					// polynomialParameters
					polynomialParameters[0] = this->axis1.polynomial[0];
					polynomialParameters[1] = this->axis1.polynomial[1];
					polynomialParameters[2] = this->axis1.polynomial[2];
					polynomialParameters[3] = this->axis2.polynomial[0];
					polynomialParameters[4] = this->axis2.polynomial[1];
					polynomialParameters[5] = this->axis2.polynomial[2];

					// mirrorOffsetParameters
					mirrorOffsetParameters[0] = this->mirrorOffset;
				}

				void fromParameterStrings(const T* const positionParameters
						, const T* const yRotationParameters
						, const T* const rotationAxisParameters
						, const T* const polynomialParameters
						, const T* const mirrorOffsetParameters) {
					// positionParameters
					this->position[0] = positionParameters[0];
					this->position[1] = positionParameters[1];
					this->position[2] = positionParameters[2];

					// y rotation
					this->rotationY = yRotationParameters[0];

					// rotationAxisParameters
					{
						this->axis1.rotationAxis = ofxCeres::VectorMath::polarToCartesian(glm::tvec3<T>(
								1
								, rotationAxisParameters[0]
								, rotationAxisParameters[1]
							));

						this->axis2.rotationAxis = ofxCeres::VectorMath::polarToCartesian(glm::tvec3<T>(
								1
								, rotationAxisParameters[2]
								, rotationAxisParameters[3]
							));
					}

					// polynomialParameters
					this->axis1.polynomial[0] = polynomialParameters[0];
					this->axis1.polynomial[1] = polynomialParameters[1];
					this->axis1.polynomial[2] = polynomialParameters[2];
					this->axis2.polynomial[0] = polynomialParameters[3];
					this->axis2.polynomial[1] = polynomialParameters[4];
					this->axis2.polynomial[2] = polynomialParameters[5];

					// mirrorOffsetParameters
					this->mirrorOffset = mirrorOffsetParameters[0];
				}
			};

			template<typename T>
			struct AxisAngles {
				T axis1;
				T axis2;
			};

			template<typename T>
			static
				ofxCeres::Models::Plane<T>
				getMirrorPlane(const AxisAngles<T>& axisAngles
				, const Parameters<T>& parameters) {

				auto mirrorTransform = glm::translate<T>(parameters.position)
					* glm::rotate<T>(parameters.rotationY * DEG_TO_RAD
						, glm::tvec3<T>(0, 1, 0))
					* glm::rotate<T>(axisAngles.axis1 * DEG_TO_RAD
						, glm::normalize(parameters.axis1.rotationAxis))
					* glm::rotate<T>(axisAngles.axis2 * DEG_TO_RAD
						, glm::normalize(parameters.axis2.rotationAxis))
					* glm::translate<T>(glm::tvec3<T>(0, -parameters.mirrorOffset, 0));

				auto center = ofxCeres::VectorMath::applyTransform<T>(mirrorTransform, { 0, 0, 0 });
				auto centerPlusNormal = ofxCeres::VectorMath::applyTransform<T>(mirrorTransform, { 0, -1, 0 });
				auto normal = ofxCeres::VectorMath::normalize(centerPlusNormal - center);

				ofxCeres::Models::Plane<T> plane;
				{
					plane.center = center;
					plane.normal = normal;
				}
				return plane;
			}

			template<typename T>
			static T positionToAngle(const T& position
				, const glm::tvec3<T>& polynomial
				, const T& angleOffset)
			{
				auto correctedPosition =
					polynomial[0]
					+ position * polynomial[1]
					+ position * position * polynomial[2];
				auto angle = (correctedPosition - (T)2048.0) / (T)4096.0 * (T)360.0;
				return angle - angleOffset;
			}

			static void drawMirror(const glm::vec3& mirrorCenter
				, const glm::vec3& mirrorNormal
				, float diameter);

			class Navigator {
			public:
				struct Solution {
					AxisAngles<float> axisAngles;
				};
				typedef ofxCeres::Result<Solution> Result;

				static ofxCeres::SolverSettings defaultSolverSettings();

				static Result solveNormal(const Parameters<float>&
					, const glm::vec3& normal
					, const AxisAngles<float>& initialAngles
					, const ofxCeres::SolverSettings& solverSettings = defaultSolverSettings());

				static Result solvePointToPoint(const Parameters<float>&
					, const glm::vec3& pointA
					, const glm::vec3& pointB
					, const AxisAngles<float>& initialAngles
					, const ofxCeres::SolverSettings& solverSettings = defaultSolverSettings());

				static Result solveVectorToPoint(const Parameters<float>&
					, const glm::vec3& incidentVector
					, const glm::vec3& point
					, const AxisAngles<float>& initialAngles
					, const ofxCeres::SolverSettings& solverSettings = defaultSolverSettings());

				static bool validate(const Parameters<float>&
					, const AxisAngles<float>& axisAngles);

				// returns true if angles were altered
				static AxisAngles<float> constrainAngles(const Parameters<float>&
					, const AxisAngles<float>& initialAngles
					, bool& changed);

				static Result solveConstrained(const Parameters<float>&
					, std::function<Result(const AxisAngles<float>&)>
					, const AxisAngles<float>& initialAngles
					, bool throwIfOutsideConstraints);
			};

			class SolvePosition {
			public:
				typedef Nodes::Experiments::MirrorPlaneCapture::Dispatcher::RegisterValue Solution;
				typedef ofxCeres::Result<Solution> Result;
				static ofxCeres::SolverSettings defaultSolverSettings();

				static Result solvePosition(const float& angle
					, const float& angleOffset
					, const glm::vec3& polynomial
					, const Nodes::Experiments::MirrorPlaneCapture::Dispatcher::RegisterValue& priorPosition
					, const ofxCeres::SolverSettings& solverSettings = defaultSolverSettings());
			};

			class Calibrator {
			public:
				typedef Parameters<float> Solution;
				typedef ofxCeres::Result<Solution> Result;
				static ofxCeres::SolverSettings defaultSolverSettings();

				struct Options {
					float mirrorDiameter = 0.35f;
					bool fixPosition = false;
					bool fixRotationY = true;
					bool fixRotationAxis = true;
					bool fixMirrorOffset = true;
					bool fixPolynomial = true;
				};

				static Result solveCalibration(const vector<ofxRay::Ray>& cameraRays
					, const vector<glm::vec3>& worldPoints
					, const vector<int>& axis1ServoPosition
					, const vector<int>& axis2ServoPosition
					, const vector<float>& axis1AngleOffset
					, const vector<float>& axis2AngleOffset
					, const Parameters<float>& priorParameters
					, const Options &
					, const ofxCeres::SolverSettings & = Calibrator::defaultSolverSettings());

				static float getResidual(const ofxRay::Ray & cameraRay
					, const glm::vec3 & worldPoint
					, int axis1ServoPosition
					, int axis2ServoPosition
					, float axis1AngleOffset
					, float axis2AngleOffset
					, const Parameters<float>& hamParameters
					, float mirrorDiameter);
			};
		};
	}
}
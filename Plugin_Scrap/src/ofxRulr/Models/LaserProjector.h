#pragma once

#include "Line.h"
#include "Transform.h"
#include "Camera.h"
#include "Ray.h"

namespace ofxRulr {
	namespace Models {
		template<typename T>
		struct LaserProjector_ {
			Transform_<T> rigidBodyTransform;
			glm::tvec2<T> fov{ (T)0, (T)0 };
			glm::tvec2<T> fov2{ (T)0, (T)0 }; // second order fov param

			glm::tvec2<T> projectionPointToAngles(const glm::tvec2<T>& projectionPoint)
			{
				return {
					projectionPoint.x * (this->fov.x / (T)2 * DEG_TO_RAD)
					+ projectionPoint.x * projectionPoint.x * this->fov2.x
					, projectionPoint.y * (this->fov.y / (T)2 * DEG_TO_RAD)
					+ projectionPoint.y * projectionPoint.y * this->fov2.y
				};
			}

			Ray_<T> castRayObjectSpace(const glm::tvec2<T>& projectionPoint)
			{
				auto angles = this->projectionPointToAngles(projectionPoint);
				auto direction = glm::rotateY(glm::tvec3<T>(0, 0, 1), angles.x);
				direction = glm::rotateX(direction, angles.y);
				return Ray_<T> {
					glm::tvec3<T>()
					, direction
				};
			}

			Ray_<T> castRayWorldSpace(const glm::tvec2<T>& projectionPoint)
			{
				const auto rayObjectSpace = castRayObjectSpace(projectionPoint);
				const auto projectorPosition = this->rigidBodyTransform.translation;
				const auto rayTransmission = this->rigidBodyTransform.applyTransform(rayObjectSpace.t) - projectorPosition;
				return Ray_<T> {
					projectorPosition
					, rayTransmission
				};
			}

			template<typename T2>
			LaserProjector_<T2> castTo() const
			{
				LaserProjector_<T2> castLaserProjector;
				castLaserProjector.rigidBodyTransform = this->rigidBodyTransform.castTo<T2>();
				castLaserProjector.fov = (glm::tvec2<T2>) this->fov;
				castLaserProjector.fov2 = (glm::tvec2<T2>) this->fov2;
				return castLaserProjector;
			}
		};

		typedef LaserProjector_<float> LaserProjector;
	}
}
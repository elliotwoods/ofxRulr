#pragma once

#include "Transform.h"

namespace ofxRulr {
	namespace Models {
		template<typename T>
		struct Intrinsics_ {
			T width;
			T height;
			glm::tmat4x4<T> projectionMatrix;

			glm::tvec2<T> viewToImage(const glm::tvec3<T>& viewPoint) 
			{
				auto projectedPoint = ofxCeres::VectorMath::applyTransform(this->projectionMatrix, viewPoint);
				return glm::tvec2<T>(
					(T)this->width * (projectedPoint.x + (T)1.0) / (T)2.0
					, (T)this->height * ((T)1.0 - projectedPoint.y) / (T)2.0
					);
			}

			template<typename T2>
			Intrinsics_<T2> castTo() const
			{
				Intrinsics_<T2> castIntrinsics;

				castIntrinsics.width = (T2)this->width;
				castIntrinsics.height = (T2)this->height;
				castIntrinsics.projectionMatrix = (glm::tmat4x4<T>)this->projectionMatrix;

				return castIntrinsics;
			}
		};

		typedef Intrinsics_<float> Intrinsics;

		template<typename T>
		struct Camera_ {
			Camera_(const Transform_<T>& viewTransform
				, const Intrinsics_<T>& intrinsics)
				: viewTransform(viewTransform)
				, intrinsics(intrinsics)
			{

			}

			Transform_<T> viewTransform;
			Intrinsics_<T> intrinsics;

			glm::tvec2<T> worldToImage(const glm::tvec3<T> & worldPoint)
			{
				auto viewPoint = this->viewTransform.applyTransform(worldPoint);
				return this->intrinsics.viewToImage(viewPoint);
			}

			template<typename T2>
			Camera_<T2> castTo() const
			{
				Camera_<T2> castCamera;

				castCamera.viewTransform = this->viewTransform.castTo<T2>();
				castCamera.intrinsics = this->intrinsics.castTo<T2>();

				return castCamera;
			}
		};

		typedef Camera_<float> Camera;
	}
}
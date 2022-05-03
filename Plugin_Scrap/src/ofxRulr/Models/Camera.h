#pragma once

#include "Transform.h"
#include "Intrinsics.h"
#include "Line.h"
#include "Ray.h"

namespace ofxRulr {
	namespace Models {
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

			glm::tvec2<T> worldToImage(const glm::tvec3<T> & worldPoint) const
			{
				auto viewPoint = this->viewTransform.applyTransform(worldPoint);
				return this->intrinsics.viewToImage(viewPoint);
			}

			Line_<T> worldToImage(const Ray_<T>& worldRay) const
			{
				auto lineS = this->worldToImage(worldRay.s);
				auto lineT = glm::normalize(this->worldToImage(worldRay.s + worldRay.t) - lineS);
				return Line_<T>(lineS, lineT);
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
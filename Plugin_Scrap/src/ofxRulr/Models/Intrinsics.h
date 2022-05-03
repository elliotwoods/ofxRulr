#pragma once

namespace ofxRulr {
	namespace Models {
		template<typename T>
		struct Intrinsics_ {
			T width;
			T height;
			glm::tmat4x4<T> projectionMatrix;

			glm::tvec2<T> viewToImage(const glm::tvec3<T>& viewPoint) const
			{
				auto projectedPoint = ofxCeres::VectorMath::applyTransform(this->projectionMatrix, viewPoint);
				return glm::tvec2<T>(
					(T)this->width * (projectedPoint.x + (T)1.0) / (T)2.0
					, (T)this->height * ((T)1.0 - projectedPoint.y) / (T)2.0
					);
			}

			glm::tvec2<T> getCenter() const
			{
				return {
					this->width / (T)2.0
					, this->height / (T)2.0
				};
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
	}
}
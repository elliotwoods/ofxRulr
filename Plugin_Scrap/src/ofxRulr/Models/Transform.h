#pragma once

#include "ofxCeres.h"
#include <glm/gtx/matrix_decompose.hpp>

namespace ofxRulr {
	namespace Models {
		template<typename T>
		struct Transform_ {
			Transform_(const T* const translationParameters
				, const T* const rotationParameters)
				: translation(translationParameters[0]
					, translationParameters[1]
					, translationParameters[2])
				, rotation(rotationParameters[0]
					, rotationParameters[1]
					, rotationParameters[2])
			{

			}

			Transform_(const glm::tvec3<T> & translation
				, const glm::tvec3<T> & rotation)
				: translation(translation)
				, rotation(rotation)
			{

			}

			Transform_(const glm::tmat4x4<T> & matrix) {
				glm::vec3 scale;
				glm::quat orientation;
				glm::vec3 translation, skew;
				glm::vec4 perspective;

				glm::decompose(matrix
					, scale
					, orientation
					, translation
					, skew
					, perspective);
				this->translation = (glm::tvec3<T>) translation;
				this->rotation = (glm::tvec3<T>) glm::eulerAngles(orientation);
			}

			Transform_() {

			}

			glm::tvec3<T> translation;
			glm::tvec3<T> rotation;

			glm::tmat4x4<T> getTransform() const
			{
				return ofxCeres::VectorMath::createTransform(this->translation, this->rotation);
			}

			glm::tvec3<T> applyTransform(const glm::tvec3<T>& point) const
			{
				return ofxCeres::VectorMath::applyTransform(this->getTransform()
					, point);
			}

			template<typename T2>
			Transform_<T2> castTo() const
			{
				return Transform_<T2>((glm::tvec3<T2>) this->translation
					, (glm::tvec3<T2>) this->rotation);
			}
		};

		typedef Transform_<float> Transform;
	}
}
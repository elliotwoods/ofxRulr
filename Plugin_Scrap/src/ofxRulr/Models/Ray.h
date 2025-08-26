#pragma once

namespace ofxRulr {
	namespace Models {
		template<typename T>
		struct Ray_ {
			glm::tvec3<T> s;
			glm::tvec3<T> t;

			glm::tvec3<T> closestPointOnRayTo(const glm::tvec3<T>& point) const
			{
				// presume that t is normalised
				auto u = ofxCeres::VectorMath::dot(point - this->s, this->t);
				return this->s + u * this->t;
			}

			T distanceTo(const glm::tvec3<T>& point) const
			{
				return ofxCeres::VectorMath::distance(point, this->closestPointOnRayTo(point));
			}

			template<typename T2>
			Ray_<T2> castTo() const
			{
				return Ray_<T2>{
					(glm::tvec3<T2>) this->s
						, (glm::tvec3<T2>) this->t
				};
			}

			ofxRay::Ray getRay() const
			{
				return ofxRay::Ray((glm::vec3)this->s
					, (glm::vec3)this->t
					, false);
			}
		};

		typedef Ray_<float> Ray;
	}
}
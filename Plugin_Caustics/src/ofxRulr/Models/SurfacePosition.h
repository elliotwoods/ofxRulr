#pragma once

namespace ofxRulr {
	namespace Models {
		template<typename T>
		struct SurfacePosition_ : DistortedGridPosition_<T>
		{
			glm::tvec3<T> target;
			glm::tvec3<T> incoming;

			glm::tvec3<T> normal; // calculated value

			SurfacePosition_()
			{
			}

			void serialize(nlohmann::json& json) const {
				Utils::serialize(json, "initialPosition", initialPosition);
				Utils::serialize(json, "rightVector", rightVector);
				Utils::serialize(json, "downVector", downVector);
				Utils::serialize(json, "currentPosition", currentPosition);

				Utils::serialize(json, "target", target);
				Utils::serialize(json, "incoming", incoming);
				Utils::serialize(json, "normal", normal);
			};

			void deserialize(const nlohmann::json& json) {
				Utils::deserialize(json, "initialPosition", initialPosition);
				Utils::deserialize(json, "rightVector", rightVector);
				Utils::deserialize(json, "downVector", downVector);
				Utils::deserialize(json, "currentPosition", currentPosition);

				Utils::deserialize(json, "target", target);
				Utils::deserialize(json, "incoming", incoming);
				Utils::deserialize(json, "normal", normal);
			};

			T dz_dx() const
			{
				return tan(atan2(normal.z, normal.x) - acos(0));
			}

			T dz_dy() const
			{
				return tan(atan2(normal.z, normal.y) - acos(0));
			}

			T getHeight() const
			{
				return this->currentPosition.z;
			}

			void setHeight(T height)
			{
				this->currentPosition.z = height;
			}

			template<typename T2>
			SurfacePosition_<T2> castTo() const
			{
				SurfacePosition_<T2> newPosition;

				newPosition.target = (glm::tvec3<T2>) this->target;
				newPosition.incoming = (glm::tvec3<T2>) this->incoming;
				newPosition.normal = (glm::tvec3<T2>) this->normal;

				newPosition.initialPosition = (glm::tvec3<T2>) this->initialPosition;
				newPosition.rightVector = (glm::tvec3<T2>) this->rightVector;
				newPosition.downVector = (glm::tvec3<T2>) this->downVector;
				newPosition.currentPosition = (glm::tvec3<T2>) this->currentPosition;

				return newPosition;
			}
		};
	}
}
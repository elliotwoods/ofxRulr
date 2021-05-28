#include "pch_RulrCore.h"
#include "oF.h"
#include "Native.h"

namespace ofxRulr {
	namespace Utils {
#pragma mark ofxRay
		//----------
		void serialize(nlohmann::json& json, const ofxRay::Ray& value) {
			Utils::serialize(json, "s", value.s);
			Utils::serialize(json, "t", value.t);
			Utils::serialize(json, "color", value.color);
			Utils::serialize(json, "define", value.defined);
			Utils::serialize(json, "infinite", value.infinite);
		}

		//----------
		bool deserialize(const nlohmann::json& json, ofxRay::Ray& value) {
			if (json.is_object()) {
				Utils::deserialize(json, "s", value.s);
				Utils::deserialize(json, "t", value.t);
				Utils::deserialize(json, "color", value.color);
				Utils::deserialize(json, "define", value.defined);
				Utils::deserialize(json, "infinite", value.infinite);
				return true;
			}
			else {
				return false;
			}
		}

		//----------
		void serialize(nlohmann::json& json, const ofxRay::Plane& value) {
			stringstream ss;
			ss << value;
			json = ss.str();
		}

		//----------
		bool deserialize(const nlohmann::json& json, ofxRay::Plane& value) {
			std::string text;
			if (Utils::deserialize(json, text)) {
				stringstream ss(text);
				ss >> value;
				return true;
			}
			return false;
		}

		//----------
		void
			serialize(nlohmann::json& json, const ofxRay::Camera& camera)
		{
			serialize(json, "orientation", camera.getOrientationQuat());
			serialize(json, "position", camera.getPosition());
			serialize(json, "scale", camera.getScale());
			serialize(json, "width", camera.getWidth());
			serialize(json, "height", camera.getHeight());
			serialize(json, "throwRatio", camera.getThrowRatio());
			serialize(json, "lensOffset", camera.getLensOffset());
			serialize(json, "nearClip", camera.getNearClip());
			serialize(json, "farClip", camera.getFarClip());
			serialize(json, "projectionMatrix", camera.getProjectionMatrix());
			serialize(json, "color", camera.color);

		}

		//----------
		bool
			deserialize(const nlohmann::json& json, ofxRay::Camera& camera)
		{
			bool useful = false;

			{
				glm::quat orientation;
				if (deserialize(json, "orientation", orientation)) {
					camera.setOrientation(orientation);
					useful = true;
				}
			}

			{
				glm::vec3 position;
				if (deserialize(json, "position", position)) {
					camera.setPosition(position);
					useful = true;
				}
			}

			{
				glm::vec3 scale;
				if (deserialize(json, "scale", scale)) {
					camera.setScale(scale);
					useful = true;
				}
			}

			{
				float width;
				if (deserialize(json, "width", width)) {
					camera.setWidth(width);
					useful = true;
				}
			}

			{
				float height;
				if (deserialize(json, "height", height)) {
					camera.setHeight(height);
					useful = true;
				}
			}

			{
				float throwRatio;
				glm::vec2 lensOffset;
				if (deserialize(json, "throwRatio", throwRatio)
					&& deserialize(json, "lensOffset", lensOffset)) {
					camera.setProjection(throwRatio, lensOffset);
					useful = true;
				}
			}

			{
				float nearClip;
				if (deserialize(json, "nearClip", nearClip)) {
					camera.setNearClip(nearClip);
					useful = true;
				}
			}

			{
				float farClip;
				if (deserialize(json, "farClip", farClip)) {
					camera.setFarClip(farClip);
					useful = true;
				}
			}

			{
				glm::mat4 projectionMatrix;
				if (deserialize(json, "projectionMatrix", projectionMatrix)) {
					camera.setProjection(projectionMatrix);
					useful = true;
				}
			}

			{
				ofColor color;
				if (deserialize(json, "color", color)) {
					camera.color = color;
					useful = true;
				}
			}

			return useful;
		}
	}
}

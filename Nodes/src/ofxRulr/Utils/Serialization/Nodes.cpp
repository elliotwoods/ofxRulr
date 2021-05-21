#include "pch_RulrNodes.h"

namespace ofxRulr {
	namespace Utils {
#pragma mark ofxRay
		//----------
		void serialize(nlohmann::json& json, const ofxRay::Ray & value) {
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
	}
}

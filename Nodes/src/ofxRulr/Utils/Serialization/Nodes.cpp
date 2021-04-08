#include "pch_RulrNodes.h"

namespace ofxRulr {
	namespace Utils {
#pragma mark ofxRay
		//----------
		void serialize(nlohmann::json& json, const ofxRay::Ray & value) {
			Utils::serialize(json["s"], value.s);
			Utils::serialize(json["t"], value.t);
			Utils::serialize(json["color"], value.color);
			Utils::serialize(json["define"], value.defined);
		}

		//----------
		bool deserialize(nlohmann::json& json, ofxRay::Ray& value) {
			if (json.is_object()) {
				Utils::serialize(json["s"], value.s);
				Utils::serialize(json["t"], value.t);
				Utils::serialize(json["color"], value.color);
				Utils::serialize(json["define"], value.defined);
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
		bool deserialize(nlohmann::json& json, ofxRay::Plane& value) {
			stringstream ss(json.get<std::string>());
			ss >> value;
		}
	}
}

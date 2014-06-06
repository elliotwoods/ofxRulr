#pragma once

#include <json/json.h>

namespace ofxDigitalEmulsion {
	namespace Utils {
		class Serializable {
		public:
			virtual void serialize(Json::Value &) = 0;
			virtual void deserialize(Json::Value &) = 0;
		};
	}
}
#pragma once

#include "ofParameter.h"
#include <json/json.h>

#include <string>

namespace ofxDigitalEmulsion {
	namespace Utils {
		class Serializable {
		public:
			virtual std::string getTypeName() const = 0;
			virtual void serialize(Json::Value &) = 0;
			virtual void deserialize(Json::Value &) = 0;

			void save(std::string filename = "") const;
			void load(std::string filename = "");
			std::string getDefaultFilename() const;
		protected:
			static void serialize(ofParameter<float> &, Json::Value &);
			static void deserialize(ofParameter<float> &, Json::Value &);
		};
	}
}
#pragma once

#include "ofParameter.h"
#include <json/json.h>

#include <string>

namespace ofxDigitalEmulsion {
	namespace Utils {
		class Serializable {
		public:
			virtual std::string getTypeName() const = 0;
			virtual std::string getName() const;
			virtual void serialize(Json::Value &) = 0;
			virtual void deserialize(const Json::Value &) = 0;

			void save(std::string filename = "");
			void load(std::string filename = "");
			std::string getDefaultFilename() const;
		protected:
			static void serialize(const ofParameter<float> &, Json::Value &);
			static void deserialize(ofParameter<float> &, const Json::Value &);
		};
	}
}
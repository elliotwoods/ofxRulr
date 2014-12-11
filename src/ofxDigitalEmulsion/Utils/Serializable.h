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
		
			static void serialize(const ofParameter<int> &, Json::Value &);
			static void serialize(const ofParameter<float> &, Json::Value &);
			static void serialize(const ofParameter<bool> &, Json::Value &);
			static void deserialize(ofParameter<int> &, const Json::Value &);
			static void deserialize(ofParameter<float> &, const Json::Value &);
			static void deserialize(ofParameter<bool> &, const Json::Value &);
		};
	}
}

template<typename T>
Json::Value & operator<< (Json::Value & json, const T & streamSerializableObject) {
	stringstream stream;
	stream << streamSerializableObject;
	json = stream.str();
	return json;
}

template<typename T>
const Json::Value & operator>> (const Json::Value & json, T & streamSerializableObject) {
	stringstream stream(json.asString());
	stream >> streamSerializableObject;
	return json;
}
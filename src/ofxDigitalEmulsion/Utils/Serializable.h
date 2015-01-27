#pragma once

#include "ofParameter.h"
#include "ofxLiquidEvent.h"

#include <json/json.h>
#include <string>

namespace ofxDigitalEmulsion {
	namespace Utils {
		class Serializable {
		public:
			virtual std::string getTypeName() const = 0;
			virtual std::string getName() const;

			ofxLiquidEvent<Json::Value> onSerialize;
			ofxLiquidEvent<const Json::Value> onDeserialize;
			void serialize(Json::Value &);
			void deserialize(const Json::Value &);

			void save(std::string filename = "");
			void load(std::string filename = "");
			std::string getDefaultFilename() const;
		
			static void serialize(const ofParameter<int> &, Json::Value &);
			static void serialize(const ofParameter<float> &, Json::Value &);
			static void serialize(const ofParameter<bool> &, Json::Value &);
			static void serialize(const ofParameter<string> &, Json::Value &);
			static void deserialize(ofParameter<int> &, const Json::Value &);
			static void deserialize(ofParameter<float> &, const Json::Value &);
			static void deserialize(ofParameter<bool> &, const Json::Value &);
			static void deserialize(ofParameter<string> &, const Json::Value &);
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
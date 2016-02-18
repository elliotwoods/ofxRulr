#pragma once

#include "ofParameter.h"
#include "ofxLiquidEvent.h"

#include <json/json.h>
#include <string>
#include <type_traits>

namespace ofxRulr {
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
		
			template<typename T>
			static void serialize(const ofParameter<T> & parameter, Json::Value & json) {
				json[parameter.getName()] << parameter.get();
			}
			static void serialize(const ofParameter<int> &, Json::Value &);
			static void serialize(const ofParameter<float> &, Json::Value &);
			static void serialize(const ofParameter<bool> &, Json::Value &);
			static void serialize(const ofParameter<string> &, Json::Value &);

			template<typename T>
			static void deserialize(ofParameter<T> & parameter, const Json::Value & json) {
				auto & jsonValue = json[parameter.getName()];
				if (!jsonValue.isNull()) {
					T value;
					jsonValue >> value;
					parameter.set(value);
				}
			}
			static void deserialize(ofParameter<int> &, const Json::Value &);
			static void deserialize(ofParameter<float> &, const Json::Value &);
			static void deserialize(ofParameter<bool> &, const Json::Value &);
			static void deserialize(ofParameter<string> &, const Json::Value &);
		};
	}
}



//--
// Parameters
//--
//
// json >> parameter; //deserialize
// json << parameter; //serialize
template<typename T>
void operator>> (const Json::Value & json, ofParameter<T> & parameter) {
	ofxRulr::Utils::Serializable::deserialize(parameter, json);
}

template<typename T>
void operator<< (Json::Value & json, const ofParameter<T> & parameter) {
	ofxRulr::Utils::Serializable::serialize(parameter, json);
}
//
//--



//--
// Raw values
//--
//
// json >> value; //deserialize
// json << value; //serialize
//
// since the json object is useless after the stream, we return void
template<class T,
	typename = std::enable_if_t<!std::is_base_of<ofAbstractParameter, T>::value> >
void operator>> (const Json::Value & json, T & streamSerializableObject) {
	stringstream stream(json.asString());
	stream >> streamSerializableObject;
}

template<class T,
	typename = std::enable_if_t<!std::is_base_of<ofAbstractParameter, T>::value> >
	void operator<< (Json::Value & json, const T & streamSerializableObject) {
	stringstream stream;
	stream << streamSerializableObject;
	json = stream.str();
}
//
//--
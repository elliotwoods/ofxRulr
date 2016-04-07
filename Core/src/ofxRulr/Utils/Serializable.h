#pragma once

#include "ofParameter.h"
#include "ofxLiquidEvent.h"

#include <json/json.h>
#include <string>
#include <type_traits>

// Syntactic sugar which enables struct-ofParameterGroup
#define PARAM_DECLARE(NAME, ...) bool paramDeclareConstructor \
{ [this] { this->setName(NAME), this->add(__VA_ARGS__); return true; }() };

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
		
			//////////////////////////////////////////////////////////////////////////

			template<typename Number>
			static void serializeNumber(const ofParameter<Number> & parameter, Json::Value & json) {
				const auto & value = parameter.get();
				if (value == value) { // don't serialize a NaN
					json[parameter.getName()] = parameter.get();
				}
			}
#define SERIALIZE_NUMBER(Type) static void serialize(const ofParameter<Type> & parameter, Json::Value & json) { serializeNumber(parameter, json); }
			SERIALIZE_NUMBER(uint8_t);
			SERIALIZE_NUMBER(uint16_t);
			SERIALIZE_NUMBER(uint32_t);
			SERIALIZE_NUMBER(uint64_t);
			SERIALIZE_NUMBER(int8_t);
			SERIALIZE_NUMBER(int16_t);
			SERIALIZE_NUMBER(int32_t);
			SERIALIZE_NUMBER(int64_t);
			SERIALIZE_NUMBER(bool);
			SERIALIZE_NUMBER(float);
			SERIALIZE_NUMBER(double);
			
			static void serialize(const ofParameter<string> &, Json::Value &);
			static void serialize(const ofParameterGroup &, Json::Value &);
			
			template<typename NotNumber,
				typename = enable_if<!is_arithmetic<NotNumber>::value, bool>::type>
			static void serialize(const ofParameter<NotNumber> & parameter, Json::Value & json) {
				json[parameter.getName()] << parameter.get();
			}

			//////////////////////////////////////////////////////////////////////////

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
			static void deserialize(ofParameterGroup &, const Json::Value &);
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
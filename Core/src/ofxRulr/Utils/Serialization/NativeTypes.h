
#include <nlohmann/json.hpp>
#include <string>
#include <type_traits>

#include "../Constants.h"

#pragma once


namespace ofxRulr {
	namespace Utils {
#define DECLARE_SERIALIZE_VAR(TYPE) \
		OFXRULR_API_ENTRY void serialize(nlohmann::json&, const TYPE &); \
		OFXRULR_API_ENTRY bool deserialize(const nlohmann::json&, TYPE &);

#define DEFINE_SERIALIZE_VAR(TYPE) \
		void serialize(nlohmann::json& json, const TYPE& value) { _serialize(json, value); } \
		bool deserialize(const nlohmann::json& json, TYPE& value) { return _deserialize(json, value); }

		//--
		// Raw values
		//--
		//
		//
		DECLARE_SERIALIZE_VAR(bool)
		DECLARE_SERIALIZE_VAR(uint8_t)
		DECLARE_SERIALIZE_VAR(uint16_t)
		DECLARE_SERIALIZE_VAR(uint32_t)
		DECLARE_SERIALIZE_VAR(uint64_t)
		DECLARE_SERIALIZE_VAR(int8_t)
		DECLARE_SERIALIZE_VAR(int16_t)
		DECLARE_SERIALIZE_VAR(int32_t)
		DECLARE_SERIALIZE_VAR(int64_t)
#ifdef TARGET_OSX
		DECLARE_SERIALIZE_VAR(long)
		DECLARE_SERIALIZE_VAR(unsigned long)
#endif
		DECLARE_SERIALIZE_VAR(float)
		DECLARE_SERIALIZE_VAR(double)
		DECLARE_SERIALIZE_VAR(std::string)
		DECLARE_SERIALIZE_VAR(filesystem::path)
		//
		//--

	}
}
#include "pch_RulrCore.h"
#include "Native.h"

namespace ofxRulr {
	namespace Utils {
		template<typename T>
		void
			_serialize(nlohmann::json& json, const T& value)
		{
			json = value;
		}

		template<typename T>
		bool
			_deserialize(const nlohmann::json& json, T& value)
		{
			if (!json.is_null()) {
				try {
					json.get_to(value);
					return true;
				}
				RULR_CATCH_ALL_TO_ERROR;
			}
			return false;
		}

		DEFINE_SERIALIZE_VAR(bool);
		DEFINE_SERIALIZE_VAR(uint8_t);
		DEFINE_SERIALIZE_VAR(uint16_t);
		DEFINE_SERIALIZE_VAR(uint32_t);
		DEFINE_SERIALIZE_VAR(uint64_t);
		DEFINE_SERIALIZE_VAR(int8_t);
		DEFINE_SERIALIZE_VAR(int16_t);
		DEFINE_SERIALIZE_VAR(int32_t);
		DEFINE_SERIALIZE_VAR(int64_t);
		DEFINE_SERIALIZE_VAR(float);
		DEFINE_SERIALIZE_VAR(double);
		DEFINE_SERIALIZE_VAR(std::string);

		void
			serialize(nlohmann::json& json, const filesystem::path& path)
		{
			json = path.string();
		}

		bool
			deserialize(const nlohmann::json& json, filesystem::path& value)
		{
			std::string valueString;
			if (!json.is_null()) {
				value = filesystem::path(json.get<string>());
				return true;
			}
			else {
				return false;
			}
		}
	}
}

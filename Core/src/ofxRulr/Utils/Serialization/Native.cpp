#include "pch_RulrCore.h"
#include "Native.h"

template<typename T>
void
serialize(nlohmann::json & json, const T & value)
{
	json = value;
}

template<typename T>
bool
deserialize(const nlohmann::json& json, T& value)
{
	if (!json.is_null()) {
		json.get_to(value);
		return true;
	}
	return false;
}

#define DEFINE_SERIALIZE_VAR_BASIC(TYPE) \
template void serialize(nlohmann::json& json, const TYPE &); \
template bool deserialize(const nlohmann::json& json, TYPE &);


DEFINE_SERIALIZE_VAR_BASIC(bool);
DEFINE_SERIALIZE_VAR_BASIC(uint8_t);
DEFINE_SERIALIZE_VAR_BASIC(uint16_t);
DEFINE_SERIALIZE_VAR_BASIC(uint32_t);
DEFINE_SERIALIZE_VAR_BASIC(uint64_t);
DEFINE_SERIALIZE_VAR_BASIC(int8_t);
DEFINE_SERIALIZE_VAR_BASIC(int16_t);
DEFINE_SERIALIZE_VAR_BASIC(int32_t);
DEFINE_SERIALIZE_VAR_BASIC(int64_t);
DEFINE_SERIALIZE_VAR_BASIC(float);
DEFINE_SERIALIZE_VAR_BASIC(double);
DEFINE_SERIALIZE_VAR_BASIC(std::string);

template<>
void
serialize(nlohmann::json& json, const filesystem::path& path)
{
	json = path.string();
}

template<>
bool
deserialize(const nlohmann::json& json, filesystem::path& path)
{
	std::string value;
	if (deserialize<std::string>(json, value)) {
		path = filesystem::path(value);
		return true;
	}
	else {
		return false;
	}
}
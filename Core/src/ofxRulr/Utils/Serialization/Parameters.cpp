#include "pch_RulrCore.h"
#include "Parameters.h"

namespace ofxRulr {
	namespace Utils {
#pragma mark ofParameter<Type>
		//----------
		template<typename Type>
		void serialize(nlohmann::json& json, const ofParameter<Type> & parameter) {
			serialize(json, parameter.get());
		}

		//----------
		template<typename Type>
		bool deserialize(const nlohmann::json& json, ofParameter<Type>& parameter) {
			Type value;
			if (deserialize(json, value)) {
				parameter.set(value);
				return true;
			}
			return false;
		}
		//----------
		template<>
		void serialize(nlohmann::json& json, const ofParameter<filesystem::path>& parameter) {
			serialize(json, parameter.get());
		}

		//----------
		template<>
		bool deserialize(const nlohmann::json& json, ofParameter<filesystem::path>& parameter) {
			std::string value;
			if (deserialize(json, value)) {
				parameter.set(value);
				return true;
			}
			return false;
		}

		template void serialize(nlohmann::json& json, const ofParameter<bool>& parameter);
		template void serialize(nlohmann::json& json, const ofParameter<uint8_t>& parameter);
		template void serialize(nlohmann::json& json, const ofParameter<uint16_t>& parameter);
		template void serialize(nlohmann::json& json, const ofParameter<uint32_t>& parameter);
		template void serialize(nlohmann::json& json, const ofParameter<uint64_t>& parameter);
		template void serialize(nlohmann::json& json, const ofParameter<int8_t>& parameter);
		template void serialize(nlohmann::json& json, const ofParameter<int16_t>& parameter);
		template void serialize(nlohmann::json& json, const ofParameter<int32_t>& parameter);
		template void serialize(nlohmann::json& json, const ofParameter<int64_t>& parameter);
		template void serialize(nlohmann::json& json, const ofParameter<float>& parameter);
		template void serialize(nlohmann::json& json, const ofParameter<double>& parameter);
		template void serialize(nlohmann::json& json, const ofParameter<glm::vec2>& parameter);
		template void serialize(nlohmann::json& json, const ofParameter<glm::vec3>& parameter);
		template void serialize(nlohmann::json& json, const ofParameter<glm::vec4>& parameter);
		template void serialize(nlohmann::json& json, const ofParameter<glm::mat3>& parameter);
		template void serialize(nlohmann::json& json, const ofParameter<glm::mat4>& parameter);
		template void serialize(nlohmann::json& json, const ofParameter<glm::quat>& parameter);
		template void serialize(nlohmann::json& json, const ofParameter<ofColor>& parameter);
		template void serialize(nlohmann::json& json, const ofParameter<ofShortColor>& parameter);
		template void serialize(nlohmann::json& json, const ofParameter<ofFloatColor>& parameter);
		template void serialize(nlohmann::json& json, const ofParameter<ofRectangle>& parameter);
		template void serialize(nlohmann::json& json, const ofParameter<std::string>& parameter);
		template void serialize(nlohmann::json& json, const ofParameter<filesystem::path>& parameter);

		template bool deserialize(const nlohmann::json& json, ofParameter<bool>& parameter);
		template bool deserialize(const nlohmann::json& json, ofParameter<uint8_t>& parameter);
		template bool deserialize(const nlohmann::json& json, ofParameter<uint16_t>& parameter);
		template bool deserialize(const nlohmann::json& json, ofParameter<uint32_t>& parameter);
		template bool deserialize(const nlohmann::json& json, ofParameter<uint64_t>& parameter);
		template bool deserialize(const nlohmann::json& json, ofParameter<int8_t>& parameter);
		template bool deserialize(const nlohmann::json& json, ofParameter<int16_t>& parameter);
		template bool deserialize(const nlohmann::json& json, ofParameter<int32_t>& parameter);
		template bool deserialize(const nlohmann::json& json, ofParameter<int64_t>& parameter);
		template bool deserialize(const nlohmann::json& json, ofParameter<float>& parameter);
		template bool deserialize(const nlohmann::json& json, ofParameter<double>& parameter);
		template bool deserialize(const nlohmann::json& json, ofParameter<glm::vec2>& parameter);
		template bool deserialize(const nlohmann::json& json, ofParameter<glm::vec3>& parameter);
		template bool deserialize(const nlohmann::json& json, ofParameter<glm::vec4>& parameter);
		template bool deserialize(const nlohmann::json& json, ofParameter<glm::mat3>& parameter);
		template bool deserialize(const nlohmann::json& json, ofParameter<glm::mat4>& parameter);
		template bool deserialize(const nlohmann::json& json, ofParameter<glm::quat>& parameter);
		template bool deserialize(const nlohmann::json& json, ofParameter<ofColor>& parameter);
		template bool deserialize(const nlohmann::json& json, ofParameter<ofShortColor>& parameter);
		template bool deserialize(const nlohmann::json& json, ofParameter<ofFloatColor>& parameter);
		template bool deserialize(const nlohmann::json& json, ofParameter<ofRectangle>& parameter);
		template bool deserialize(const nlohmann::json& json, ofParameter<std::string>& parameter);
		template bool deserialize(const nlohmann::json& json, ofParameter<filesystem::path>& parameter);

#pragma mark ofParameterGroup
		//----------
		template<typename Type>
		bool trySerialize(nlohmann::json& json, shared_ptr<ofAbstractParameter> abstractParameter) {
			auto typedParameter = dynamic_pointer_cast<ofParameter<Type>>(abstractParameter);
			if (typedParameter) {
				serialize(json, * typedParameter);
				return true;
			}
			else {
				return false;
			}
		}

		//----------
		void serialize(nlohmann::json& json, const ofParameterGroup& group) {
			const auto name = group.getName();
			auto& jsonGroup = name.empty() ? json : json[name];
			for (const auto& parameter : group) {
				if (trySerialize<bool>(jsonGroup, parameter)) continue;
				if (trySerialize<uint8_t>(jsonGroup, parameter)) continue;
				if (trySerialize<uint16_t>(jsonGroup, parameter)) continue;
				if (trySerialize<uint32_t>(jsonGroup, parameter)) continue;
				if (trySerialize<uint64_t>(jsonGroup, parameter)) continue;
				if (trySerialize<int8_t>(jsonGroup, parameter)) continue;
				if (trySerialize<int16_t>(jsonGroup, parameter)) continue;
				if (trySerialize<int32_t>(jsonGroup, parameter)) continue;
				if (trySerialize<int64_t>(jsonGroup, parameter)) continue;
				if (trySerialize<float>(jsonGroup, parameter)) continue;
				if (trySerialize<double>(jsonGroup, parameter)) continue;

				if (trySerialize<glm::vec2>(jsonGroup, parameter)) continue;
				if (trySerialize<glm::vec3>(jsonGroup, parameter)) continue;
				if (trySerialize<glm::vec4>(jsonGroup, parameter)) continue;
				if (trySerialize<glm::mat3>(jsonGroup, parameter)) continue;
				if (trySerialize<glm::mat4>(jsonGroup, parameter)) continue;

				if (trySerialize<ofColor>(jsonGroup, parameter)) continue;
				if (trySerialize<ofShortColor>(jsonGroup, parameter)) continue;
				if (trySerialize<ofFloatColor>(jsonGroup, parameter)) continue;
				if (trySerialize<ofRectangle>(jsonGroup, parameter)) continue;

				if (trySerialize<string>(jsonGroup, parameter)) continue;
				if (trySerialize<filesystem::path>(jsonGroup, parameter)) continue;

				//group
				{
					auto typedParameter = dynamic_pointer_cast<ofParameterGroup>(parameter);
					if (typedParameter) {
						serialize(jsonGroup, *typedParameter);
						continue;
					}
				}

				//anything else
				{
					jsonGroup[name] = parameter->toString();
					RULR_ERROR << "Can't serialize contents of " << name;
					continue;
				}

				ofLogWarning("ofxRulr::Utils::serialize") << "Couldn't serialize" << parameter->getName();
			}
		}

		//----------
		template<typename Type>
		bool tryDeserialize(const nlohmann::json& json, shared_ptr<ofAbstractParameter> abstractParameter) {
			auto typedParameter = dynamic_pointer_cast<ofParameter<Type>>(abstractParameter);
			if (typedParameter) {
				deserialize(json, *typedParameter);
				return true;
			}
			else {
				return false;
			}
		}

		//----------
		bool deserialize(const nlohmann::json& json, ofParameterGroup& group) {
			const auto name = group.getName();
			auto& jsonGroup = name.empty() ? json : json[name];
			for (const auto& parameter : group) {
				if (!parameter) {
					continue;
				}

				if (tryDeserialize<int>(jsonGroup, parameter)) continue;
				if (tryDeserialize<float>(jsonGroup, parameter)) continue;
				if (tryDeserialize<bool>(jsonGroup, parameter)) continue;

				if (tryDeserialize<uint8_t>(jsonGroup, parameter)) continue;
				if (tryDeserialize<uint16_t>(jsonGroup, parameter)) continue;
				if (tryDeserialize<uint32_t>(jsonGroup, parameter)) continue;
				if (tryDeserialize<uint64_t>(jsonGroup, parameter)) continue;

				if (tryDeserialize<int8_t>(jsonGroup, parameter)) continue;
				if (tryDeserialize<int16_t>(jsonGroup, parameter)) continue;
				if (tryDeserialize<int32_t>(jsonGroup, parameter)) continue;
				if (tryDeserialize<int64_t>(jsonGroup, parameter)) continue;

				if (tryDeserialize<glm::vec2>(jsonGroup, parameter)) continue;
				if (tryDeserialize<glm::vec3>(jsonGroup, parameter)) continue;
				if (tryDeserialize<glm::vec4>(jsonGroup, parameter)) continue;
				if (tryDeserialize<glm::mat3>(jsonGroup, parameter)) continue;
				if (tryDeserialize<glm::mat4>(jsonGroup, parameter)) continue;

				if (tryDeserialize<ofColor>(jsonGroup, parameter)) continue;
				if (tryDeserialize<ofShortColor>(jsonGroup, parameter)) continue;
				if (tryDeserialize<ofFloatColor>(jsonGroup, parameter)) continue;
				if (tryDeserialize<ofRectangle>(jsonGroup, parameter)) continue;

				if (tryDeserialize<string>(jsonGroup, parameter)) continue;
				if (tryDeserialize<filesystem::path>(jsonGroup, parameter)) continue;

				//group
				{
					auto typedParameter = dynamic_pointer_cast<ofParameterGroup>(parameter);
					if (typedParameter) {
						deserialize(jsonGroup, *typedParameter);
						continue;
					}
				}

				//anything else
				{
					const auto name = parameter->getName();
					if (jsonGroup[name].is_object()) {
						string valueString;
						jsonGroup[name].get_to(valueString);
						parameter->fromString(valueString);
						continue;
					}
				}

				ofLogWarning("ofxRulr::Utils::serialize") << "Couldn't deserialize" << parameter->getName();
			}

			return true;
		}
	}
}
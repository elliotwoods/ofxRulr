#include "pch_RulrCore.h"
#include "Parameters.h"

namespace ofxRulr {
	namespace Utils {
#pragma mark ofParameter<Type>
		//----------
		template<typename Type>
		void _serialize(nlohmann::json& json, const ofParameter<Type> & parameter) {
			serialize(json[parameter.getName()], parameter.get());
		}

		//----------
		template<typename Type>
		bool _deserialize(const nlohmann::json& json, ofParameter<Type>& parameter) {
			Type value;
			const auto& name = parameter.getName();
			if (json.contains(name)) {
				if (deserialize(json[name], value)) {
					parameter.set(value);
					return true;
				}
			}
			return false;
		}
		//----------
		template<>
		void _serialize(nlohmann::json& json, const ofParameter<filesystem::path>& parameter) {
			serialize(json, parameter.get());
		}

		//----------
		template<>
		bool _deserialize(const nlohmann::json& json, ofParameter<filesystem::path>& parameter) {
			std::string value;
			if (deserialize(json, value)) {
				parameter.set(value);
				return true;
			}
			return false;
		}

		DEFINE_SERIALIZE_VAR(ofParameter<bool>);
		DEFINE_SERIALIZE_VAR(ofParameter<uint8_t>);
		DEFINE_SERIALIZE_VAR(ofParameter<uint16_t>);
		DEFINE_SERIALIZE_VAR(ofParameter<uint32_t>);
		DEFINE_SERIALIZE_VAR(ofParameter<uint64_t>);
		DEFINE_SERIALIZE_VAR(ofParameter<int8_t>);
		DEFINE_SERIALIZE_VAR(ofParameter<int16_t>);
		DEFINE_SERIALIZE_VAR(ofParameter<int32_t>);
		DEFINE_SERIALIZE_VAR(ofParameter<int64_t>);
		DEFINE_SERIALIZE_VAR(ofParameter<float>);
		DEFINE_SERIALIZE_VAR(ofParameter<double>);
		DEFINE_SERIALIZE_VAR(ofParameter<glm::vec2>);
		DEFINE_SERIALIZE_VAR(ofParameter<glm::vec3>);
		DEFINE_SERIALIZE_VAR(ofParameter<glm::vec4>);
		DEFINE_SERIALIZE_VAR(ofParameter<glm::mat3>);
		DEFINE_SERIALIZE_VAR(ofParameter<glm::mat4>);
		DEFINE_SERIALIZE_VAR(ofParameter<glm::quat>);
		DEFINE_SERIALIZE_VAR(ofParameter<ofColor>);
		DEFINE_SERIALIZE_VAR(ofParameter<ofShortColor>);
		DEFINE_SERIALIZE_VAR(ofParameter<ofFloatColor>);
		DEFINE_SERIALIZE_VAR(ofParameter<ofRectangle>);
		DEFINE_SERIALIZE_VAR(ofParameter<std::string>);
		DEFINE_SERIALIZE_VAR(ofParameter<filesystem::path>);

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

				if (trySerialize<std::string>(jsonGroup, parameter)) continue;
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
			
			if (!name.empty() && !json.contains(name)) {
				return false;
			}

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
					if (jsonGroup.contains(name)) {
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
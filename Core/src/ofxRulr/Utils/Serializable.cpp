#include "pch_RulrCore.h"
#include "Serializable.h"

#include "../Exception.h"

using namespace std;

namespace ofxRulr {
	namespace Utils {
		//----------
		void Serializable::serialize(const ofParameter<string> & parameter, Json::Value & json) {
			json[parameter.getName()] = parameter.get();
		}

		//----------
		template<typename Type>
		bool trySerialize(Json::Value & json, shared_ptr<ofAbstractParameter> abstractParameter) {
			auto typedParameter = dynamic_pointer_cast<ofParameter<Type>>(abstractParameter);
			if (typedParameter) {
				const auto & parameter = *typedParameter;
				Serializable::serialize(parameter, json);
				return true;
			}
			else {
				return false;
			}
		}

		//----------
		void Serializable::serialize(const ofParameterGroup & group, Json::Value & json) {
			const auto name = group.getName();
			auto & jsonGroup = name.empty() ? json : json[name];
			for (const auto & parameter : group) {
				if (trySerialize<uint8_t>(jsonGroup, parameter)) continue;
				if (trySerialize<uint16_t>(jsonGroup, parameter)) continue;
				if (trySerialize<uint32_t>(jsonGroup, parameter)) continue;
				if (trySerialize<uint64_t>(jsonGroup, parameter)) continue;

				if (trySerialize<int8_t>(jsonGroup, parameter)) continue;
				if (trySerialize<int16_t>(jsonGroup, parameter)) continue;
				if (trySerialize<int32_t>(jsonGroup, parameter)) continue;
				if (trySerialize<int64_t>(jsonGroup, parameter)) continue;

				if (trySerialize<bool>(jsonGroup, parameter)) continue;
				if (trySerialize<float>(jsonGroup, parameter)) continue;
				if (trySerialize<double>(jsonGroup, parameter)) continue;

				if (trySerialize<string>(jsonGroup, parameter)) continue;

				{
					auto typedParameter = dynamic_pointer_cast<ofParameterGroup>(parameter);
					if (typedParameter) {
						serialize(*typedParameter, jsonGroup);
					}
				}
			}
		}

		//----------
		void Serializable::deserialize(ofParameter<int> & parameter, const Json::Value & json) {
			const auto name = parameter.getName();
			if (json[name].isNumeric()) {
				parameter.set(json[name].asInt());
			}
		}

		//----------
		void Serializable::deserialize(ofParameter<float> & parameter, const Json::Value & json) {
			const auto name = parameter.getName();
			if (json.isMember(name)) {
				parameter.set(json[name].asFloat());
			}
		}

		//----------
		void Serializable::deserialize(ofParameter<bool> & parameter, const Json::Value & json) {
			const auto name = parameter.getName();
			if (json.isMember(name)) {
				parameter.set(json[parameter.getName()].asBool());
			}
		}

		//----------
		void Serializable::deserialize(ofParameter<string> & parameter, const Json::Value & json) {
			const auto name = parameter.getName();
			if (json.isMember(name)) {
				parameter.set(json[parameter.getName()].asString());
			}
		}
		//----------
		template<typename Type>
		bool tryDeserialize(const Json::Value & json, shared_ptr<ofAbstractParameter> abstractParameter) {
			auto typedParameter = dynamic_pointer_cast<ofParameter<Type>>(abstractParameter);
			if (typedParameter) {
				auto & parameter = *typedParameter;
				Serializable::deserialize(parameter, json);
				return true;
			}
			else {
				return false;
			}
		}

		//----------
		void Serializable::deserialize(ofParameterGroup & group, const Json::Value & json) {
			const auto name = group.getName();
			auto & jsonGroup = name.empty() ? json : json[name];
			for (const auto & parameter : group) {
				if (!parameter) {
					continue;
				}

				if (tryDeserialize<int>(jsonGroup, parameter)) continue;
				if (tryDeserialize<float>(jsonGroup, parameter)) continue;
				if (tryDeserialize<bool>(jsonGroup, parameter)) continue;
				if (tryDeserialize<string>(jsonGroup, parameter)) continue;
			
				if (tryDeserialize<string>(jsonGroup, parameter)) continue;

				//group
				{
					auto typedParameter = dynamic_pointer_cast<ofParameterGroup>(parameter);
					if (typedParameter) {
						deserialize(*typedParameter, jsonGroup);
						continue;
					}
				}

				//anything else
				{
					const auto name = parameter->getName();
					if (json.isMember(name)) {
						string valueString;
						jsonGroup[name] >> valueString;
						parameter->fromString(valueString);
					}
				}
			}
		}

		//----------
		string Serializable::getName() const {
			return this->getTypeName();
		}

		//----------
		void Serializable::serialize(Json::Value & json) {
			this->onSerialize.notifyListeners(json);
		}

		//----------
		void Serializable::deserialize(const Json::Value & json) {
			this->onDeserialize.notifyListeners(json);
		}

		//----------
		void Serializable::save(string filename) {
			if (filename == "") {
				auto result = ofSystemSaveDialog(this->getDefaultFilename(), "Save " + this->getTypeName());
				if (result.bSuccess) {
					filename = result.fileName;
				}
			}

			if (filename != "") {
				Json::Value json;
				this->serialize(json);
				Json::StyledWriter writer;
				ofFile output;
				output.open(filename, ofFile::WriteOnly, false);
				output << writer.write(json);
			}
		}
		
		//----------
		void Serializable::load(string filename) {
			if (filename == "") {
				auto result = ofSystemLoadDialog("Load " + this->getTypeName());
				if (result.bSuccess) {
					filename = result.fileName;
				}
			}

			if (filename != "") {
				try {
					ofFile input;
					input.open(ofToDataPath(filename, true), ofFile::ReadOnly, false);
					string jsonRaw = input.readToBuffer().getText();

					Json::Reader reader;
					Json::Value json;
					reader.parse(jsonRaw, json);
					this->deserialize(json);
				} 
				RULR_CATCH_ALL_TO_ALERT
			}
		}

		//----------
		string Serializable::getDefaultFilename() const {
			auto name = this->getName();
			std::replace(name.begin(), name.end(), ':', '_');
			return name + ".json";
		}
	}
}
#include "pch_RulrCore.h"
#include "Serializable.h"

#include "../Exception.h"

using namespace std;

namespace ofxRulr {
	namespace Utils {
		//----------
		void Serializable::serialize(Json::Value & json, const ofParameter<string> & parameter) {
			json[parameter.getName()] = parameter.get();
		}

		//----------
		template<typename Type>
		bool trySerialize(Json::Value & json, shared_ptr<ofAbstractParameter> abstractParameter) {
			auto typedParameter = dynamic_pointer_cast<ofParameter<Type>>(abstractParameter);
			if (typedParameter) {
				const auto & parameter = *typedParameter;
				Serializable::serialize(json, parameter);
				return true;
			}
			else {
				return false;
			}
		}

		//----------
		void Serializable::serialize(Json::Value & json, const ofParameterGroup & group) {
			const auto name = group.getName();
			auto & jsonGroup = name.empty() ? json : json[name];
			for (const auto & parameter : group) {
				if (trySerialize<int>(jsonGroup, parameter)) continue;
				if (trySerialize<float>(jsonGroup, parameter)) continue;
				if (trySerialize<bool>(jsonGroup, parameter)) continue;

				if (trySerialize<ofVec2f>(jsonGroup, parameter)) continue;
				if (trySerialize<ofVec3f>(jsonGroup, parameter)) continue;
				if (trySerialize<ofVec4f>(jsonGroup, parameter)) continue;

				if (trySerialize<string>(jsonGroup, parameter)) continue;

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
					const auto name = parameter->getName();
					jsonGroup[name] << parameter->toString();
					continue;
				}

				ofLogWarning("ofxRulr::Utils::Serializable::serialize") << "Couldn't serialize" << parameter->getName();
			}
		}

		//----------
		void Serializable::deserialize(const Json::Value & json, ofParameter<int> & parameter) {
			const auto name = parameter.getName();
			if (json[name].isNumeric()) {
				parameter.set(json[name].asInt());
			}
		}

		//----------
		void Serializable::deserialize(const Json::Value & json, ofParameter<float> & parameter) {
			const auto name = parameter.getName();
			if (json.isMember(name)) {
				parameter.set(json[name].asFloat());
			}
		}

		//----------
		void Serializable::deserialize(const Json::Value & json, ofParameter<bool> & parameter) {
			const auto name = parameter.getName();
			if (json.isMember(name)) {
				parameter.set(json[parameter.getName()].asBool());
			}
		}

		//----------
		void Serializable::deserialize(const Json::Value & json, ofParameter<string> & parameter) {
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
				Serializable::deserialize(json, parameter);
				return true;
			}
			else {
				return false;
			}
		}

		//----------
		void Serializable::deserialize(const Json::Value & json, ofParameterGroup & group) {
			const auto name = group.getName();
			auto & jsonGroup = name.empty() ? json : json[name];
			for (const auto & parameter : group) {
				if (!parameter) {
					continue;
				}

				if (tryDeserialize<int>(jsonGroup, parameter)) continue;
				if (tryDeserialize<float>(jsonGroup, parameter)) continue;
				if (tryDeserialize<bool>(jsonGroup, parameter)) continue;

				if (tryDeserialize<ofVec2f>(jsonGroup, parameter)) continue;
				if (tryDeserialize<ofVec3f>(jsonGroup, parameter)) continue;
				if (tryDeserialize<ofVec4f>(jsonGroup, parameter)) continue;
			
				if (tryDeserialize<string>(jsonGroup, parameter)) continue;

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
					if (jsonGroup.isMember(name)) {
						string valueString;
						jsonGroup[name] >> valueString;
						parameter->fromString(valueString);
						continue;
					}
				}

				ofLogWarning("ofxRulr::Utils::Serializable::serialize") << "Couldn't deserialize" << parameter->getName();
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
#include "Serializable.h"
#include "Exception.h"

#include "ofSystemUtils.h"
#include <string>
using namespace std;

namespace ofxDigitalEmulsion {
	namespace Utils {
		//----------
		void Serializable::serialize(const ofParameter<int> & parameter, Json::Value & json) {
			json[parameter.getName()] = parameter.get();
		}

		//----------
		void Serializable::serialize(const ofParameter<float> & parameter, Json::Value & json) {
			json[parameter.getName()] = parameter.get();
		}

		//----------
		void Serializable::serialize(const ofParameter<bool> & parameter, Json::Value & json) {
			json[parameter.getName()] = parameter.get();
		}

		//----------
		void Serializable::serialize(const ofParameter<string> & parameter, Json::Value & json) {
			json[parameter.getName()] = parameter.get();
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
				OFXDIGITALEMULSION_CATCH_ALL_TO_ALERT
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
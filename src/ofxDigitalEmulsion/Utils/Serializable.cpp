#include "Serializable.h"

#include "ofSystemUtils.h"
#include <string>

using namespace std;

namespace ofxDigitalEmulsion {
	namespace Utils {
		//----------
		void Serializable::serialize(const ofParameter<float> & parameter, Json::Value & json) {
			json[parameter.getName()] = parameter.get();
		}

		//----------
		void Serializable::serialize(const ofParameter<bool> & parameter, Json::Value & json) {
			json[parameter.getName()] = parameter.get();
		}

		//----------
		void Serializable::deserialize(ofParameter<float> & parameter, const Json::Value & json) {
			const auto name = parameter.getName();
			if (json[name].isNumeric()) {
				parameter.set(json[name].asFloat());
			}
		}

		//----------
		void Serializable::deserialize(ofParameter<bool> & parameter, const Json::Value & json) {
			const auto name = parameter.getName();
			if (json[name].isBool()) {
				parameter.set(json[parameter.getName()].asBool());
			}
		}

		//----------
		string Serializable::getName() const {
			return this->getTypeName();
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
				} catch (std::exception e) {
					ofSystemAlertDialog(e.what());
				}
			}
		}

		//----------
		string Serializable::getDefaultFilename() const {
			return this->getName() + ".json";
		}
	}
}
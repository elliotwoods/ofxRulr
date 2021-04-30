#include "pch_RulrCore.h"
#include "Serializable.h"

#include "../Exception.h"

using namespace std;

namespace ofxRulr {
	namespace Utils {
		//----------
		string Serializable::getName() const {
			return this->getTypeName();
		}

		//----------
		void Serializable::serialize(nlohmann::json& json) {
			this->onSerialize.notifyListeners(json);
		}

		//----------
		void Serializable::deserialize(const nlohmann::json& json) {
			this->onDeserialize.notifyListeners(json);
		}

		//----------
		void Serializable::save(string filename) {
			if (filename == "") {
				auto result = ofSystemSaveDialog(this->getDefaultFilename() + ".json", "Save " + this->getTypeName());
				if (result.bSuccess) {
					filename = result.fileName;
				}
			}

			if (filename != "") {
				nlohmann::json json;
				this->serialize(json);
				ofFile output;
				output.open(filename, ofFile::WriteOnly, false);
				output << json.dump(4);
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
					if (ofFile::doesFileExist(filename)) {
						ofFile input;
						input.open(ofToDataPath(filename, true), ofFile::ReadOnly, false);
						string jsonRaw = input.readToBuffer().getText();

						auto json = nlohmann::json::parse(jsonRaw);
						this->deserialize(json);
					}
				}
				RULR_CATCH_ALL_TO_ALERT
			}
		}

		//----------
		string Serializable::getDefaultFilename() const {
			auto name = this->getName();
			std::replace(name.begin(), name.end(), ':', '_');
			return name;
		}
	}
}

//----------

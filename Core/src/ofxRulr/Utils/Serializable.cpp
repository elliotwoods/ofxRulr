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
				// We write to a temporary file first and then copy across
				// This avoids emptying the output file and not saving anything (e.g. in case of exception whlilst serialising)

				auto tempFilename = filename + "-temp";

				nlohmann::json json;
				this->serialize(json);

				{
					ofFile output;
					output.open(tempFilename, ofFile::WriteOnly, false);
					output << json.dump(4);
					output.close();
				}

				if (ofFile::doesFileExist(filename)) {
					// delete the old file
					ofFile::removeFile(filename);
				}

				// Copy the temp file to the correct file
				if (ofFile::copyFromTo(tempFilename, filename, true)) {
					ofFile::removeFile(tempFilename);
				}

				if (json.empty()) {
					throw(ofxRulr::Exception("Serialization failed"));
				}
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

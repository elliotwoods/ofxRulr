#pragma once

#include "NativeTypes.h"
#include "oF.h"
#include "Parameters.h"
#include "Addons.h"

namespace ofxRulr {
	namespace Utils {
		//--
		// Vectors of things
		//--
		//
		// json >> vector<value>; //deserialize
		// json << vector<value>; //serialize
		//
		template<class DataType>
		void serialize(nlohmann::json& json, const vector<DataType>& vectorOfStreamSerializableObjects) {
			json = nlohmann::json::array();
			for (int i = 0; i < vectorOfStreamSerializableObjects.size(); i++) {
				serialize(json[i], vectorOfStreamSerializableObjects[i]);
			}
		}

		template<class DataType>
		bool deserialize(const nlohmann::json& json, vector<DataType>& vectorOfStreamSerializableObjects) {
			if (json.is_array()) {
				vectorOfStreamSerializableObjects.clear();
				for (const auto& jsonItem : json) {
					DataType value;
					deserialize(jsonItem, value);
					vectorOfStreamSerializableObjects.push_back(value);
				}
				return true;
			}
			else {
				return false;
			}
		}

		template<class Type>
		void serialize(nlohmann::json& json, const std::string& address, const Type& value) {
			serialize(json[address], value);
		}

		template<class Type>
		bool deserialize(const nlohmann::json& json, const std::string& address, Type& value) {
			if (!json.contains(address)) {
				return false;
			}
			return deserialize(json[address], value);
		}
		//
		//--
	}
}

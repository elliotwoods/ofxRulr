#pragma once

#include "ofParameter.h"
#include "ofxLiquidEvent.h"

#include <nlohmann/json.hpp>
#include <string>
#include <type_traits>

#include "ofxRulr/Utils/Constants.h"
#include "ofxRulr/Exception.h"

#include "Serialization/Native.h"
#include "Serialization/oF.h"
#include "Serialization/Parameters.h"

#define RULR_SERIALIZE_LISTENERS \
	this->onSerialize += [this](nlohmann::json & json) { \
		this->serialize(json); \
	}; \
	this->onDeserialize += [this](nlohmann::json const & json) { \
		this->deserialize(json); \
	}

namespace ofxRulr {
	namespace Utils {
		class OFXRULR_API_ENTRY Serializable {
		public:
			virtual std::string getTypeName() const = 0;
			virtual std::string getName() const;

			ofxLiquidEvent<nlohmann::json> onSerialize;
			ofxLiquidEvent<const nlohmann::json> onDeserialize;
			void serialize(nlohmann::json &);
			void deserialize(const nlohmann::json &);

			void save(std::string filename = "");
			void load(std::string filename = "");
			std::string getDefaultFilename() const;
		};
	}
}

template<typename Type>
void operator<<(nlohmann::json& json, const Type& value)
{
	ofxRulr::Utils::serialize(json, value);
}

template<typename Type>
void operator>>(const nlohmann::json& json, Type& value)
{
	ofxRulr::Utils::deserialize(json, value);
}
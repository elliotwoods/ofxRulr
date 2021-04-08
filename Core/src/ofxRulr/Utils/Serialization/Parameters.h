#pragma once

#include "Native.h"
#include "oF.h"
#include "ofParameter.h"

#include <type_traits>

// For parameters, we tend to use the 'serialize' rather than '<<' style

namespace ofxRulr {
	namespace Utils {
		//--
		// Parameters
		//--
		//
		DECLARE_SERIALIZE_VAR(ofParameter<bool>);
		DECLARE_SERIALIZE_VAR(ofParameter<uint8_t>);
		DECLARE_SERIALIZE_VAR(ofParameter<uint16_t>);
		DECLARE_SERIALIZE_VAR(ofParameter<uint32_t>);
		DECLARE_SERIALIZE_VAR(ofParameter<uint64_t>);
		DECLARE_SERIALIZE_VAR(ofParameter<int8_t>);
		DECLARE_SERIALIZE_VAR(ofParameter<int16_t>);
		DECLARE_SERIALIZE_VAR(ofParameter<int32_t>);
		DECLARE_SERIALIZE_VAR(ofParameter<int64_t>);
		DECLARE_SERIALIZE_VAR(ofParameter<float>);
		DECLARE_SERIALIZE_VAR(ofParameter<double>);
		DECLARE_SERIALIZE_VAR(ofParameter<glm::vec2>);
		DECLARE_SERIALIZE_VAR(ofParameter<glm::vec3>);
		DECLARE_SERIALIZE_VAR(ofParameter<glm::vec4>);
		DECLARE_SERIALIZE_VAR(ofParameter<glm::mat3>);
		DECLARE_SERIALIZE_VAR(ofParameter<glm::mat4>);
		DECLARE_SERIALIZE_VAR(ofParameter<glm::quat>);
		DECLARE_SERIALIZE_VAR(ofParameter<ofColor>);
		DECLARE_SERIALIZE_VAR(ofParameter<ofShortColor>);
		DECLARE_SERIALIZE_VAR(ofParameter<ofFloatColor>);
		DECLARE_SERIALIZE_VAR(ofParameter<ofRectangle>);
		DECLARE_SERIALIZE_VAR(ofParameter<std::string>);
		DECLARE_SERIALIZE_VAR(ofParameter<filesystem::path>);

		DECLARE_SERIALIZE_VAR(ofParameterGroup);
		//
		//--
	}
}

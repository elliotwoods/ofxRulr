#pragma once

#include <string.h>
#include <chrono>

#include "ofxRulr/Utils/Constants.h"

namespace ofxRulr {
	namespace Utils {
		OFXRULR_API_ENTRY string formatDuration(chrono::duration<double, ratio<1,1>>
			, bool showHours = true
			, bool showMinutes = true
			, bool showSeconds = true
			, bool showMilliseconds = false);

		OFXRULR_API_ENTRY glm::vec3 applyTransform(const glm::mat4&, const glm::vec3&);
		OFXRULR_API_ENTRY glm::vec2 applyTransform(const glm::mat4&, const glm::vec2&);

		OFXRULR_API_ENTRY void speakCount(size_t);
	}
}
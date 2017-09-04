#pragma once

#include <string.h>
#include <chrono>

#include "ofxRulr/Utils/Constants.h"

namespace ofxRulr {
	namespace Utils {
		RULR_EXPORTS string formatDuration(chrono::duration<double, ratio<1,1>>
			, bool showHours = true
			, bool showMinutes = true
			, bool showSeconds = true
			, bool showMilliseconds = false);
	}
}
#pragma once

#include <string.h>
#include <chrono>

#include "ofxRulr/Utils/Constants.h"

namespace ofxRulr {
	namespace Utils {
		RULR_EXPORTS string formatDuration(chrono::system_clock::duration, bool showHours = true, bool showMinutes = true, bool showSeconds = true, bool showMilliseconds = false);
	}
}
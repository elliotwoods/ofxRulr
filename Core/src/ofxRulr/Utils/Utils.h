#pragma once

#include <string.h>
#include <chrono>

namespace ofxRulr {
	namespace Utils {
		string formatDuration(chrono::system_clock::duration, bool showHours = true, bool showMinutes = true, bool showSeconds = true, bool showMilliseconds = false);
	}
}
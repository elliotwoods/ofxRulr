#include "pch_RulrCore.h"
#include "Utils.h"

namespace ofxRulr {
	namespace Utils {
		//----------
		string Utils::formatDuration(chrono::system_clock::duration duration
		, bool showHours
		, bool showMinutes
		, bool showSeconds
		, bool showMilliseconds) {
			stringstream output;

			if (showHours) {
				auto hours = chrono::duration_cast<chrono::hours>(duration);
				duration -= hours;
				if (hours.count() > 0) {
					output << hours.count() << "h ";
				}
			}

			if (showMinutes) {
				auto minutes = chrono::duration_cast<chrono::minutes>(duration);
				duration -= minutes;
				if (minutes.count() > 0) {
					output << minutes.count() << "m ";
				}
			}

			if (showSeconds) {
				auto seconds = chrono::duration_cast<chrono::seconds>(duration);
				duration -= seconds;
				if (seconds.count() > 0) {
					output << seconds.count() << "s ";
				}
			}

			if (showMilliseconds) {
				auto milliseconds = chrono::duration_cast<chrono::milliseconds>(duration);
				duration -= milliseconds;
				if (milliseconds.count() > 0) {
					output << milliseconds.count() << "ms";
				}
			}

			return output.str();
		}
	}
}

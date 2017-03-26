#pragma once

#include "ofxRulr/Utils/Constants.h"

namespace ofxRulr {
	namespace Utils {
		class RULR_EXPORTS Graphics {
		public:
			static void pushPointSize(float pointSize, bool smoothed = true);
			static void popPointSize();
		};
	}
}
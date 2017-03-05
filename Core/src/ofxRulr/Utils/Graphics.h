#pragma once

namespace ofxRulr {
	namespace Utils {
		class Graphics {
		public:
			static void pushPointSize(float pointSize, bool smoothed = true);
			static void popPointSize();
		};
	}
}
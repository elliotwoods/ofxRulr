#include "pch_RulrCore.h"
#include "Graphics.h"

namespace ofxRulr {
	namespace Utils {
		namespace Graphics {
			//----------
			void pushPointSize(float pointSize, bool smoothed) {
				glPushAttrib(GL_POINT_BIT);
				glPointSize(pointSize);
				glEnable(GL_POINT_SMOOTH);
			}

			//----------
			void popPointSize() {
				glPopAttrib();
			}
		}
	}
}

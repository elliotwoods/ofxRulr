#include "pch_RulrCore.h"
#include "Graphics.h"

namespace ofxRulr {
	namespace Utils {
		namespace Graphics {
			//----------
			void pushPointSize(float pointSize, bool smoothed) {
				//Need to rewrite this for GL 3.0+
				//glPushAttrib(GL_POINT_BIT);
				glPointSize(pointSize);
				if (smoothed) {
					glEnable(GL_POINT_SMOOTH);
				}
			}

			//----------
			void popPointSize() {
				//glPopAttrib();
				glPointSize(1.0f);
			}
		}
	}
}

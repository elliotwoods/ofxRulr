#include "pch_RulrCore.h"
#include "Graphics.h"

namespace ofxRulr {
	namespace Utils {
		//----------
		void Graphics::pushPointSize(float pointSize, bool smoothed) {
			//Need to rewrite this for GL 3.0+
			//glPushAttrib(GL_POINT_BIT);
			glPointSize(pointSize);
			if (smoothed) {
				glEnable(GL_POINT_SMOOTH);
			}
		}

		//----------
		void Graphics::popPointSize() {
			//glPopAttrib();
			glPointSize(1.0f);
		}

		//----------
		void Graphics::glEnable(GLenum cap) {
			::glEnable(cap);
		}

		//----------
		void Graphics::glDisable(GLenum cap) {
			::glDisable(cap);
		}

		//----------
		void Graphics::glPushAttrib(GLbitfield mask) {
			::glPushAttrib(mask);
		}

		//----------
		void Graphics::glPopAttrib() {
			::glPopAttrib();
		}
	}
}

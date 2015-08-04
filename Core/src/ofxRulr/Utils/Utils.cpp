#include "Utils.h"

//----------
OFXSINGLETON_DEFINE(ofxRulr::Utils::Assets);

namespace ofxRulr {
	namespace Utils {
		//----------
		Assets::Assets() {
			this->failSound.load("assets/ofxRulr/sound/fail.mp3");
			this->successSound.load("assets/ofxRulr/sound/success.mp3");
		}

		//----------
		void playSuccessSound() {
			try {
				Assets::X().successSound.play();
			} catch (...) {
				ofLogWarning("ofxRulr") << "Failed to play success sound";
			}
		}

		//----------
		void playFailSound() {
			try {
				Assets::X().failSound.play();
			} catch (...) {
				ofLogWarning("ofxRulr") << "Failed to play fail sound";
			}
		}
	}
}
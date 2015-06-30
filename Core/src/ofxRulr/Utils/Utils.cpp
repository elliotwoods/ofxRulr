#include "Utils.h"

namespace ofxRulr {
	namespace Utils {
		//----------
		Assets * Assets::singleton = nullptr;

		//----------
		Assets::Assets() {
			this->failSound.load("assets/ofxRulr/sound/fail.mp3");
			this->successSound.load("assets/ofxRulr/sound/success.mp3");
		}

		//----------
		Assets & Assets::X() {
			if (!Assets::singleton) {
				Assets::singleton = new Assets();
			}
			return * Assets::singleton;
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
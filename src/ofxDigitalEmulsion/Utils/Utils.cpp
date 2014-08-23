#include "Utils.h"

namespace ofxDigitalEmulsion {
	namespace Utils {
		//----------
		Assets * Assets::singleton = nullptr;

		//----------
		Assets::Assets() {
			this->failSound.loadSound("fail.mp3");
			this->successSound.loadSound("success.mp3");
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
				ofLogWarning("ofxDigitalEmulsion") << "Failed to play success sound";
			}
		}

		//----------
		void playFailSound() {
			try {
				Assets::X().failSound.play();
			} catch (...) {
				ofLogWarning("ofxDigitalEmulsion") << "Failed to play fail sound";
			}
		}
	}
}
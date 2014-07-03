#include "Utils.h"

namespace ofxDigitalEmulsion {
	namespace Utils {
		//----------
		Assets assetRegister = Assets();

		//----------
		Assets::Assets() {
			this->failSound.loadSound("fail.mp3");
			this->successSound.loadSound("success.mp3");
		}

		//----------
		void playSuccessSound() {
			try {
				assetRegister.successSound.play();
			} catch (...) {
				ofLogWarning("ofxDigitalEmulsion") << "Failed to play success sound";
			}
		}

		//----------
		void playFailSound() {
			try {
				assetRegister.failSound.play();
			} catch (...) {
				ofLogWarning("ofxDigitalEmulsion") << "Failed to play fail sound";
			}
		}
	}
}
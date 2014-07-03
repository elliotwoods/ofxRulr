#pragma once

#include "ofSoundPlayer.h"

namespace ofxDigitalEmulsion {
	namespace Utils {
		class Assets{
		public:
			Assets();
			ofSoundPlayer failSound;
			ofSoundPlayer successSound;
		};

		extern Assets assetRegister;
		void playSuccessSound();
		void playFailSound();
	}
}
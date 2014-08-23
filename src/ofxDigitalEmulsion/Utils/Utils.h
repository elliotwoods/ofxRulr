#pragma once

#include "ofSoundPlayer.h"

namespace ofxDigitalEmulsion {
	namespace Utils {
		class Assets{
		public:
			Assets();
			static Assets & X();

			ofSoundPlayer failSound;
			ofSoundPlayer successSound;

			static Assets * singleton;
		};

		void playSuccessSound();
		void playFailSound();
	}
}
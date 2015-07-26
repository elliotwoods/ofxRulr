#pragma once

#include "ofxSingleton.h"

#include "ofSoundPlayer.h"

namespace ofxRulr {
	namespace Utils {
		class Assets : public ofxSingleton::Singleton<Assets> {
		public:
			Assets();

			ofSoundPlayer failSound;
			ofSoundPlayer successSound;
		};

		void playSuccessSound();
		void playFailSound();
	}
}
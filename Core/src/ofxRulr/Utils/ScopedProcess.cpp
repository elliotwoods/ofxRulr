#include "pch_RulrCore.h"
#include "ScopedProcess.h"

namespace ofxRulr {
	namespace Utils {
		//----------
		ScopedProcess::ScopedProcess(const string & activityName) {
			SoundEngine::X().play("ofxRulr::start");
			this->idling = true;

			ofxCvGui::Utils::drawProcessingNotice(activityName);
			
			this->idleThread = thread([this]() {
				auto & soundEngine = SoundEngine::X();
				auto & assetsRegister = ofxAssets::Register::X();
				
				auto startSound = assetsRegister.getSoundPointer("ofxRulr::start");
				auto idleSound = assetsRegister.getSoundPointer("ofxRulr::idle");
				
				bool waitForStartSound = true;
				
				while(this->idling) {
					if(waitForStartSound) {
						waitForStartSound = false;
						ofSleepMillis(1000);
					} else {
						soundEngine.play(idleSound);
						ofSleepMillis(2000);
					}
				}
			});
		}

		//----------
		ScopedProcess::~ScopedProcess() {
			if(this->idling) {
				//we must have failed
				this->end(false);
			}
			this->idling = false;
			this->idleThread.join();
		}
		
		//----------
		void ScopedProcess::end(bool success) {
			this->idling = false;
			
			auto & assetsRegister = ofxAssets::Register::X();
			auto & soundEngine = SoundEngine::X();
			
			//wait for start sound to finish if it's currenlty playing
			auto startSound = assetsRegister.getSoundPointer("ofxRulr::start");
			auto delay = soundEngine.getRemainingNumFrames(startSound);
			
			//choose appropriate end sound
			auto endSound = assetsRegister.getSoundPointer(success ? "ofxRulr::success" : "ofxRulr::failure");
			
			//construct the sound with delay
			SoundEngine::ActiveSound activeSound;
			activeSound.sound = endSound;
			activeSound.delay = delay;
			
			//stop any idling sounds
			soundEngine.stopAll("ofxRulr::idle");
			
			soundEngine.play(activeSound);
		}
	}
}
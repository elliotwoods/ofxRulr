#pragma once

#include "ofxRulr/Nodes/Base.h"

#include "ofxSingleton.h"
#include "ofxAssets.h"

#include "ofSoundStream.h"

namespace ofxRulr {
	namespace Utils {
		class SoundEngine : public ofxSingleton::Singleton<SoundEngine>, public ofBaseSoundOutput {
		public:
			struct ActiveSound {
				//since we use shared_ptr, we keep the sound even if it is unloaded elsewhere
				shared_ptr<ofxAssets::Register::Sound> sound;
				size_t frameIndex = 0;
				size_t delay = 0;
			};
			
			SoundEngine();
			void audioOut(ofSoundBuffer &) override;
			
			void addSource(weak_ptr<ofBaseSoundOutput>);
			
			void play(const string & soundAssetName);
			void play(shared_ptr<ofxAssets::Register::Sound>);
			void play(ActiveSound);
			
			void stopAll(const string & soundAssetName);
			void stopAll(shared_ptr<ofxAssets::Register::Sound>);
			
			bool isSoundActive(shared_ptr<ofxAssets::Register::Sound>);
			size_t getRemainingNumFrames(shared_ptr<ofxAssets::Register::Sound>);
		protected:
			ofSoundStream soundStream;
			
			vector<weak_ptr<ofBaseSoundOutput>> sources;
			mutex sourcesMutex;
			
			vector<ActiveSound> activeSounds;
			mutex activeSoundsMutex;
			
			vector<ActiveSound> soundsToAdd;
			mutex soundsToAddMutex;
		};
	}
}
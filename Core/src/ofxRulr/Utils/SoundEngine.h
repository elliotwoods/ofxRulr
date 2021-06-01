#pragma once

#include "ofxRulr/Nodes/Base.h"

#include "ofxSingleton.h"
#include "ofxAssets.h"

#include "ofSoundStream.h"

namespace ofxRulr {
	namespace Utils {
		class OFXRULR_API_ENTRY SoundEngine : public ofxSingleton::Singleton<SoundEngine>, public ofBaseSoundOutput {
		public:
			struct ActiveSound {
				//since we use shared_ptr, we keep the sound even if it is unloaded elsewhere
				shared_ptr<ofxAssets::Sound> sound;
				size_t frameIndex = 0;
				size_t delay = 0;
			};
			
			SoundEngine();
			~SoundEngine();
			void init();
			void close();
			bool isInitialised() const;

			void audioOut(ofSoundBuffer &) override;

			void addSource(weak_ptr<ofBaseSoundOutput>);
			
			void play(const string & soundAssetName, bool onlyAllowOneAtOnce);
			void play(shared_ptr<ofxAssets::Sound>, bool onlyAllowOneAtOnce);
			void play(ActiveSound);
			
			void stopAll(const string & soundAssetName);
			void stopAll(shared_ptr<ofxAssets::Sound>);
			
			bool isSoundActive(shared_ptr<ofxAssets::Sound>);
			size_t getRemainingNumFrames(shared_ptr<ofxAssets::Sound>);
		protected:
			ofSoundStream soundStream;
			bool isOpen = false;
			bool isClosing = false;

			vector<weak_ptr<ofBaseSoundOutput>> sources;
			mutex sourcesMutex;
			
			vector<ActiveSound> activeSounds;
			mutex activeSoundsMutex;

			ofThreadChannel<ActiveSound> soundsToAdd;
			ofThreadChannel<ActiveSound> soundsToRemove;
		};
	}
}
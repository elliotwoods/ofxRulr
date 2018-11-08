#include "pch_RulrCore.h"
#include "SoundEngine.h"

OFXSINGLETON_DEFINE(ofxRulr::Utils::SoundEngine);

namespace ofxRulr {
	namespace Utils {
		//----------
		SoundEngine::SoundEngine() {
			//try default output device
			bool success = false;
#ifdef TARGET_WIN32
			auto device = ofSoundDevice::MS_DS;
#else
			auto device = ofSoundDevice::DEFAULT;
#endif
			auto devices = this->soundStream.getDeviceList(device);
			for (auto & device : devices) {
				if (device.isDefaultOutput) {
					ofSoundStreamSettings settings;
					settings.setOutDevice(device);
					settings.setInDevice(device);
					settings.numOutputChannels = 2;
					settings.numInputChannels = 0;
					settings.sampleRate = 44100;
					settings.bufferSize = 1024;
					settings.numBuffers = 1;
					this->soundStream.setup(settings);
				}
			}
			//if that didn't work
			success = this->soundStream.getNumOutputChannels() > 0;
			//try again. just look for any that works
			if (!success) {
				for (auto & device : devices) {
					if (device.outputChannels >= 2) {
						ofSoundStreamSettings settings;
						settings.setOutDevice(device);
						settings.setInDevice(device);
						settings.numOutputChannels = 2;
						settings.numInputChannels = 0;
						settings.sampleRate = 44100;
						settings.bufferSize = 1024;
						settings.numBuffers = 1;
						this->soundStream.setup(settings);
						success = true;
						break;
					}
				}
			}
			if (!success) {
				ofLogError("ofxRulr::Utils::SondEngine") << "No workable sound output device detected. If you are on Windows you may need to install ASIO4All";
			}

			this->soundStream.setOutput(this);
		}

		//----------
		SoundEngine::~SoundEngine() {
			this->soundStream.close();
		}
		
		//----------
		void SoundEngine::audioOut(ofSoundBuffer & out) {
			auto numFrames = out.getNumFrames();

			//add all sounds in queue to activeSounds list and clear the list
			this->soundsToAddMutex.lock();
			for(const auto & soundToAdd : this->soundsToAdd) {
				this->activeSounds.push_back(soundToAdd);
			}
			this->soundsToAdd.clear();
			this->soundsToAddMutex.unlock();
			
			this->activeSoundsMutex.lock();
			{
				//render all the frames of sound and delete if completed
				for(size_t i=0; i<numFrames; i++) {
					auto & left = out[i * 2 + 0];
					auto & right = out[i * 2 + 1];
					
					//initialise each channel
					left = 0.0f;
					right = 0.0f;
					
					//delete all dead sounds and play active ones
					for(auto it = this->activeSounds.begin(); it != this->activeSounds.end(); ) {
						auto & activeSound = *it;
						
						//check if needs deleting
						if(activeSound.frameIndex >= activeSound.sound->getSoundBuffer().getNumFrames()) {
							it = this->activeSounds.erase(it);
							continue; // skip to next sound
						} else {
							it++;
						}
						
						//check if it's delayed
						if(activeSound.delay > 0) {
							activeSound.delay--;
							continue; //skip to next sound
						}
						
						//play it otherwise
						left += activeSound.sound->getSoundBuffer().getSample(activeSound.frameIndex, 0);
						right += activeSound.sound->getSoundBuffer().getSample(activeSound.frameIndex, 1);
						
						//progress the playhead
						activeSound.frameIndex++;
					}
				}
			}
			this->activeSoundsMutex.unlock();
			
			this->sourcesMutex.lock();
			{
				for(auto it = this->sources.begin(); it != this->sources.end(); ) {
					auto source = it->lock();
					if(!source) {
						it = this->sources.erase(it);
						continue;
					} else {
						source->audioOut(out);
					}
					it++;
				}
			}
			this->sourcesMutex.unlock();
		}
		
		//----------
		void SoundEngine::addSource(weak_ptr<ofBaseSoundOutput> source) {
			this->sourcesMutex.lock();
			{
				this->sources.push_back(source);
			}
			this->sourcesMutex.unlock();
		}
		
		//----------
		void SoundEngine::play(const string & soundAssetName) {
			auto sound = ofxAssets::Register::X().getSoundPointer(soundAssetName);
			this->play(sound);
		}
		
		//----------
		void SoundEngine::play(shared_ptr<ofxAssets::Sound> sound) {
			if(sound) {
				ActiveSound activeSound;
				activeSound.sound = sound;
				
				this->play(activeSound);
			}
		}
		
		//----------
		void SoundEngine::play(ActiveSound activeSound) {
			//HACK
			return;

			if (this->soundsToAddMutex.try_lock()) {
				this->soundsToAdd.push_back(activeSound);
				this->soundsToAddMutex.unlock();
			}
		}

		//----------
		void SoundEngine::stopAll(const string & soundAssetName) {
			auto soundAsset = ofxAssets::Register::X().getSoundPointer(soundAssetName);
			this->stopAll(soundAsset);
		}
		
		//----------
		void SoundEngine::stopAll(shared_ptr<ofxAssets::Sound> sound) {
			if (sound) {
				if (this->soundsToAddMutex.try_lock()) {
					for (auto it = this->soundsToAdd.begin(); it != this->soundsToAdd.end(); ) {
						if (it->sound == sound) {
							it = this->soundsToAdd.erase(it);
						}
						else {
							it++;
						}
					}

					this->soundsToAddMutex.unlock();
				}
				
				if (this->activeSoundsMutex.try_lock()) {
					for(auto it = this->activeSounds.begin(); it != this->activeSounds.end(); ) {
						if(it->sound == sound) {
							it = this->activeSounds.erase(it);
						} else {
							it++;
						}
					}
					this->activeSoundsMutex.unlock();
				}
			}
		}
		
		//----------
		bool SoundEngine::isSoundActive(shared_ptr<ofxAssets::Sound> sound) {
			lock_guard<mutex> lock(this->activeSoundsMutex);
			
			for(const auto & activeSound : this->activeSounds) {
				if(activeSound.sound == sound) {
					return true;
				}
			}
			
			return false;
		}

		//----------
		size_t SoundEngine::getRemainingNumFrames(shared_ptr<ofxAssets::Sound> sound) {
			auto numFrames = sound->getSoundBuffer().getNumFrames();

			size_t result = 0;
			
			this->soundsToAddMutex.lock();
			{
				for(const auto & activeSound : this->soundsToAdd) {
					if(activeSound.sound == sound) {
						result = max(result, numFrames - activeSound.frameIndex);
					}
				}
				
			}
			this->soundsToAddMutex.unlock();
			
			this->activeSoundsMutex.lock();
			{
				for(const auto & activeSound : this->activeSounds) {
					if(activeSound.sound == sound) {
						result = max(result, numFrames - activeSound.frameIndex);
					}
				}
			}
			this->activeSoundsMutex.unlock();
			
			return result;
		}
	}
}
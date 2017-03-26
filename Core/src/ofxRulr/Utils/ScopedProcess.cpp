#include "pch_RulrCore.h"
#include "ScopedProcess.h"

#include "ofxRulr/Utils/Utils.h"

OFXSINGLETON_DEFINE(ofxRulr::Utils::ScopedProcess::ActiveProcesses);

namespace ofxRulr {
	namespace Utils {
#pragma mark ActiveProcesses
		//----------
		ScopedProcess::ActiveProcesses::ActiveProcesses() {
			this->idlingThread = thread([this]() {
				auto & soundEngine = SoundEngine::X();
				auto & assetsRegister = ofxAssets::Register::X();

				auto startSound = assetsRegister.getSoundPointer("ofxRulr::start");
				auto idleSound = assetsRegister.getSoundPointer("ofxRulr::idle");

				while (true) {
					//if empty then wait
					if (!this->isSounding) {
						unique_lock<std::mutex> lock(this->mutex);
						this->waitVariable.wait(lock);
					}
					
					if (this->destructing) {
						return; // leave thread
					}

					if (this->waitForStartSound) {
						ofSleepMillis(1000);
						waitForStartSound = false;
					}
					else {
						soundEngine.play(idleSound);
						ofSleepMillis(2000);
					}
				}
			});
		}

		//----------
		ScopedProcess::ActiveProcesses::~ActiveProcesses() {
			this->destructing = true;
			this->waitVariable.notify_one();
			this->idlingThread.join();
		}

		//----------
		void ScopedProcess::ActiveProcesses::pushProcess(ScopedProcess * process) {
			this->activeProcesses.push_back(process);

			//start sounding
			if (!isSounding && process->getHasSuccessOrFail()) {
				SoundEngine::X().play("ofxRulr::start");
				this->waitForStartSound = true;
				this->isSounding = true;
				this->waitVariable.notify_one();
			}

			//print to screen
			{
				stringstream message;
				for (const auto process : this->activeProcesses) {
					message << process->getActivityName() << endl;
					if (process->hasCountedChildProcesses()) {
						message << "[" << process->getChildProcessActiveIndex() << "/" << process->getChildProcessCount() << "] ";
						
						float progress = (float)process->getChildProcessActiveIndex() / (float)process->getChildProcessCount();
						message << "[";
						for (float t = 0; t < 1.0f; t += 0.02f) {
							message << ((t > progress) ? " " : "-");
						}
						message << "] ";

						const auto timeSpent = chrono::system_clock::now() - process->getStartTime();
						const auto timeSpentInMillis = chrono::duration_cast<chrono::milliseconds>(timeSpent).count();
						const auto totalTimePrediced = timeSpentInMillis * (1.0f / progress);
						const auto timeRemaining = chrono::milliseconds((long long) ((1.0f - progress) * (float)totalTimePrediced));
						if (timeRemaining > chrono::milliseconds(0)) {
							message << Utils::formatDuration(timeRemaining) << " remaining.";
						}
						message << endl;
					}
				}
				ofxCvGui::Utils::drawProcessingNotice(message.str());
			}

			//print to console
			{
				for (size_t i = 0; i < this->activeProcesses.size() - 1; i++) {
					cout << "\t";
				}

				cout << "Starting : " << process->getActivityName();

				if (process->hasCountedChildProcesses()) {
					cout << " {" << process->getChildProcessCount() << "}";
				}
				if (this->activeProcesses.size() > 1) {
					//we might be a child process
					auto parentProcess = this->activeProcesses[this->activeProcesses.size() - 2];
					if (parentProcess->hasCountedChildProcesses()) {
						cout << " [" << parentProcess->getChildProcessActiveIndex() << "/" << parentProcess->getChildProcessCount() << "]" << endl;
					}
				} 

				cout << endl;
			}
		}

		//----------
		void ScopedProcess::ActiveProcesses::popProcess(ScopedProcess * process) {
			if (process->getHasSuccessOrFail()) {
				//play the end sound
				{
					auto & assetRegister = ofxAssets::Register::X();
					auto & soundEngine = SoundEngine::X();

					//play sound
					{
						shared_ptr<ofxAssets::Sound> sound;
						if (process->wasSuccess()) {
							sound = assetRegister.getSoundPointer("ofxRulr::success");
						}
						else {
							sound = assetRegister.getSoundPointer("ofxRulr::failure");
						}
						auto delay = soundEngine.getRemainingNumFrames(assetRegister.getSoundPointer("ofxRulr::start"));

						SoundEngine::ActiveSound activeSound;
						activeSound.sound = sound;
						activeSound.delay = delay;
						soundEngine.play(activeSound);
					}

					//stop active sound
					{
						//check if any parent processes need to continue sounding
						bool hasSoundingProcess = false;
						for (auto parentProcess : this->activeProcesses) {
							if (parentProcess == process) {
								//we've gone too far up the tree
								break;
							}

							if (parentProcess->getHasSuccessOrFail()) {
								hasSoundingProcess = true;
								break;
							}
						}

						if (!hasSoundingProcess) {
							this->isSounding = false;
						}
					}
				}
			}

			//find this process and erase
			for (auto processIterator = this->activeProcesses.begin()
				; processIterator != this->activeProcesses.end()
				; processIterator++) {
				if (*processIterator == process) {
					//erase this process and everything beneath it
					this->activeProcesses.erase(processIterator, this->activeProcesses.end());
					break;
				}
			}

			//step up parent if it's counting
			if (!this->activeProcesses.empty()) {
				auto & topProcess = activeProcesses.back();
				if (topProcess->hasCountedChildProcesses()) {
					topProcess->nextChildProcess();
				}
			}

			//stop idling sound if this is the last
			if (this->activeProcesses.empty()) {
				SoundEngine::X().stopAll("ofxRulr::idle");
			}

			//print to console
			{
				for (int i = 0; i < this->activeProcesses.size(); i++) {
					cout << "\t";
				}

				cout << "Ending : " << process->getActivityName();
				if (process->getHasSuccessOrFail() && !process->wasSuccess()) {
					cout << " (FAIL)";
				}
				if (this->activeProcesses.size() > 0) {
					//we might be a child process
					auto parentProcess = this->activeProcesses[this->activeProcesses.size() - 1];
					if (parentProcess->hasCountedChildProcesses()) {
						cout << " [" << parentProcess->getChildProcessActiveIndex() << "/" << parentProcess->getChildProcessCount() << "]";
					}
				}

				auto duration = process->getDuration();
				if (duration > std::chrono::seconds(10)) {
					cout << " Completed in " << Utils::formatDuration(duration) << endl;
				}

				cout << endl;
			}
		}

		//----------
		bool ScopedProcess::ActiveProcesses::waitingForStartSound() const {
			return this->waitForStartSound;
		}

#pragma mark ScopedProcess
		//----------
		ScopedProcess::ScopedProcess(const string & activityName, bool hasSuccessOrFail) {
			this->activityName = activityName;
			this->active = false;
			this->hasSuccessOrFail = hasSuccessOrFail;
			this->startTime = chrono::system_clock::now();
			ActiveProcesses::X().pushProcess(this);
		}


		//----------
		ScopedProcess::ScopedProcess(const std::string & activityName, bool hasSuccessOrFail, size_t childProcessCount)
			: ScopedProcess(activityName, hasSuccessOrFail) {
			this->childProcessCount = childProcessCount;
			this->hasSuccessOrFail = hasSuccessOrFail;
		}

		//----------
		ScopedProcess::~ScopedProcess() {
			if(this->active) {
				//we must have failed
				this->end(false);
			}
			this->duration = chrono::system_clock::now() - this->startTime;
			ActiveProcesses::X().popProcess(this);
		}
		
		//----------
		void ScopedProcess::end(bool success) {
			this->success = success;
			this->active = false;
		}

		//----------
		bool ScopedProcess::getHasSuccessOrFail() const {
			return this->hasSuccessOrFail;
		}

		//----------
		bool ScopedProcess::wasSuccess() const {
			return this->success;
		}

		//----------
		const string & ScopedProcess::getActivityName() const {
			return this->activityName;
		}

		//----------
		const std::chrono::system_clock::time_point & ScopedProcess::getStartTime() const {
			return this->startTime;
		}

		//----------
		const std::chrono::system_clock::duration & ScopedProcess::getDuration() const {
			return this->duration;
		}

		//----------
		bool ScopedProcess::hasCountedChildProcesses() const {
			return this->childProcessCount > 0;
		}

		//----------
		void ScopedProcess::nextChildProcess() {
			this->childProcessActiveIndex++;
		}

		//----------
		size_t ScopedProcess::getChildProcessCount() const {
			return this->childProcessCount;
		}

		//----------
		size_t ScopedProcess::getChildProcessActiveIndex() const {
			return this->childProcessActiveIndex;
		}
	}
}

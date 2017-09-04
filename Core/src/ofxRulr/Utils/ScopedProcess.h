#pragma once

#include "SoundEngine.h"

#include "ofxSingleton.h"

namespace ofxRulr {
	namespace Utils {
		class RULR_EXPORTS ScopedProcess {
		public:
			class ActiveProcesses : public ofxSingleton::Singleton<ActiveProcesses> {
			public:
				ActiveProcesses();
				virtual ~ActiveProcesses();
				void pushProcess(ScopedProcess *);
				void popProcess(ScopedProcess *);
				bool waitingForStartSound() const;
			protected:
				vector<ScopedProcess *> activeProcesses;
				bool active;
				thread idlingThread;
				bool waitForStartSound;
				bool destructing = false;
				bool isSounding = false;
				std::condition_variable waitVariable;
				std::mutex mutex;
			};

			ScopedProcess(const std::string & activityName, bool hasSuccessOrFail = true);
			ScopedProcess(const std::string & activityName, bool hasSuccessOrFail, size_t childProcessCount);
			~ScopedProcess();
			
			void end(bool success = true);

			bool getHasSuccessOrFail() const;
			bool wasSuccess() const;
			const string & getActivityName() const;

			const chrono::system_clock::time_point & getStartTime() const;
			const chrono::system_clock::duration & getDuration() const;

			bool hasCountedChildProcesses() const;
			void nextChildProcess();

			size_t getChildProcessCount() const;
			size_t getChildProcessActiveIndex() const;
		protected:
			bool active = false;
			bool success = false;
			bool hasSuccessOrFail = true;
			size_t childProcessCount = 0;
			size_t childProcessActiveIndex = 0;
			string activityName;
			chrono::system_clock::time_point startTime;
			chrono::system_clock::duration duration;
		};
	}
}
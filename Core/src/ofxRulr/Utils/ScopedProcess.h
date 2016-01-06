#pragma once

#include "SoundEngine.h"

namespace ofxRulr {
	namespace Utils {
		class ScopedProcess {
		public:
			ScopedProcess(const std::string & activityName);
			~ScopedProcess();
			
			void end(bool success = true);
		protected:
			bool idling;
			
			thread idleThread;
		};
	}
}
#pragma once

#include "ofxRulr/Utils/Constants.h"

namespace ofxRulr {
	namespace Utils {
		class OFXRULR_API_ENTRY Initialiser {
		public:
			Initialiser();
			void checkInitialised();
		protected:
			void initialise();
			bool initialised;
		};

		extern Initialiser initialiser;
	}
}
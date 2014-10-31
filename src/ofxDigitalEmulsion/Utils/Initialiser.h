#pragma once

namespace ofxDigitalEmulsion {
	namespace Utils {
		class Initialiser {
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
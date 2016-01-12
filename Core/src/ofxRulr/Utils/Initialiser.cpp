#include "pch_RulrCore.h"
#include "Initialiser.h"

namespace ofxRulr {
	namespace Utils {
		//----------
		Initialiser::Initialiser() {
			this->initialised = false;
		}

		//----------
		void Initialiser::checkInitialised() {
			if (!this->initialised) {
				this->initialise();
				this->initialised = true;
			}
		}

		//----------
		void Initialiser::initialise() {
			ofxAssets::Register::X().addAddon("ofxRulr");
		}

		//----------
		Initialiser initialiser = Initialiser();
	}
}
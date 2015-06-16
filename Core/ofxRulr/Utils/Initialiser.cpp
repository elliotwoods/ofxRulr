#include "Initialiser.h"

#include "ofxAssets.h"

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
			ofxAssets::AssetRegister.addAddon("ofxRulr");
		}

		//----------
		Initialiser initialiser = Initialiser();
	}
}
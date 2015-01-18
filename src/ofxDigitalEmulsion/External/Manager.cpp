#include "Manager.h"

namespace ofxDigitalEmulsion {
	namespace External {
		//----------
		Manager * Manager::singleton = nullptr;

		//----------
		Manager & Manager::X() {
			if (!Manager::singleton) {
				Manager::singleton = new Manager();
			}
			return * Manager::singleton;
		}
		//----------
		Manager::Manager() {

		}

		//----------
		void Manager::registerFactories() {
			for (auto & factoryInitialiser : factoryInitialisers) {
				factoryInitialiser();
			}
		}
	}
}
#include "Pin.h"

namespace ofxDigitalEmulsion {
	namespace Graph {
		//----------
		BasePin::BasePin(string name) : name(name) {
		}

		//----------
		string BasePin::getName() const {
			return this->name;
		}
	}
}
#pragma once

#include "../Graph/Node.h"
#include "../Graph/Factory.h"

#include "ofParameter.h"

using namespace std;

namespace ofxDigitalEmulsion {
	namespace Item {
		class Base : public Graph::Node {
		public:
			virtual void drawWorld() { }
		};
	}
}
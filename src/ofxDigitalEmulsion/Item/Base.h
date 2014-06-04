#pragma once

#include "../Utils/Constants.h"
#include "../../../addons/ofxCvGui2/src/ofxCvGui/Widgets/IInspectable.h"
#include <string>
#include "ofParameter.h"

using namespace std;

namespace ofxDigitalEmulsion {
	namespace Item {
		class Base : public ofxCvGui::Widgets::IInspectable {
		public:
			virtual string getTypeName() const = 0;
			virtual void populate(ofxCvGui::ElementGroupPtr) { }

			virtual void update() { }
		};
	}
}
#pragma once

#include "../../../addons/ofxCvGui2/src/ofxCvGui/Widgets/IInspectable.h"
#include <string>

using namespace std;

namespace ofxDigitalEmulsion {
	namespace Device {
		class Base : public ofxCvGui::Widgets::IInspectable {
		public:
			virtual string getTypeName() const;
			virtual void populate(ofxCvGui::ElementGroupPtr) { }
		};
	}
}
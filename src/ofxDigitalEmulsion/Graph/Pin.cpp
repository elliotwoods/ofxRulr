#include "Pin.h"
#include "../../../addons/ofxCvGui/src/ofxCvGui/Utils/Utils.h"

namespace ofxDigitalEmulsion {
	namespace Graph {
		//----------
		BasePin::BasePin(string name) : name(name) {
			this->setBounds(ofRectangle(0, 0, 100, 20));
			this->onDraw += [this](ofxCvGui::DrawArguments &) {
				ofxCvGui::Utils::drawText(this->getName(), this->getBounds(), false);
			};
		}

		//----------
		string BasePin::getName() const {
			return this->name;
		}
	}
}
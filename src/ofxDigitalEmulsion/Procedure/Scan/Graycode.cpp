#include "Graycode.h"

#include "../../Item/Camera.h"

using namespace ofxDigitalEmulsion::Graph;

namespace ofxDigitalEmulsion {
	namespace Procedure {
		namespace Scan {
			//---------
			Graycode::Graycode() {
				this->inputPins.push_back(MAKE(Pin<Item::Camera>));
			}

			//----------
			string Graycode::getTypeName() const {
				return "Graycode";
			}
		}
	}
}
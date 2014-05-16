#include "Camera.h"

using namespace ofxCvGui;

namespace ofxDigitalEmulsion {
	namespace Device {
		//----------
		string Camera::getTypeName() const {
			return "Camera";
		}

		//----------
		void Camera::populate(ElementGroupPtr inspector) {
		}

		//----------
		void Camera::setGrabber(ofxMachineVision::GrabberPtr grabber) {
			this->grabber = grabber;
		}

		//----------
		ofxMachineVision::GrabberPtr Camera::getGrabber() {
			return this->grabber;
		}
	}
}
#pragma once

#include "Base.h"
#include "../../../ofxMachineVision/src/ofxMachineVision/Grabber/Base.h"

namespace ofxDigitalEmulsion {
	namespace Device {
		class Camera : public Base {
		public:
			string getTypeName() const;
			void populate(ofxCvGui::ElementGroupPtr);
			
			void setGrabber(ofxMachineVision::GrabberPtr);
			ofxMachineVision::GrabberPtr getGrabber();
		protected:
			shared_ptr<ofxMachineVision::GrabberPtr> grabber;
		};
	}
}
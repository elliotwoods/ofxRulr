#pragma once

#include "Base.h"
#include "../../../addons/ofxMachineVision/src/ofxMachineVision.h"
#include "ofParameter.h"

namespace ofxDigitalEmulsion {
	namespace Device {
		class Camera : public Base {
		public:
			Camera();
			string getTypeName() const;
			void update();
			void populate(ofxCvGui::ElementGroupPtr);

			void setDevice(ofxMachineVision::DevicePtr, int deviceIndex = 0);
			shared_ptr<ofxMachineVision::Grabber::Simple> getGrabber();
		protected:
			shared_ptr<ofxMachineVision::Grabber::Simple> grabber;

			ofParameter<float> exposure;
			ofParameter<float> gain;
			ofParameter<float> focus;

			void exposureCallback(float &);
			void gainCallback(float &);
			void focusCallback(float &);
		};
	}
}
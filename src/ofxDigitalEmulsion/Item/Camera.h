#pragma once

#include "Base.h"
#include "../../../addons/ofxMachineVision/src/ofxMachineVision.h"

namespace ofxDigitalEmulsion {
	namespace Item {
		class Camera : public Base {
		public:
			OFXDIGITALEMULSION_MAKE_ELEMENT_SIMPLE(Camera)
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
			ofParameter<float> sharpness;

			void exposureCallback(float &);
			void gainCallback(float &);
			void focusCallback(float &);
			void sharpnessCallback(float &);
		};
	}
}
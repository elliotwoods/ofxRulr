#pragma once

#include "Base.h"
#include "../../../addons/ofxMachineVision/src/ofxMachineVision.h"

#include "ofxCvMin.h"

#define OFXDIGITALEMULSION_CAMERA_DISTORTION_COEFFICIENT_COUNT 4

namespace ofxDigitalEmulsion {
	namespace Item {
		class Camera : public Base {
		public:
			Camera();
			string getTypeName() const;
			void update();
			ofxCvGui::PanelPtr getView() override;

			void serialize(Json::Value &) override;
			void deserialize(Json::Value &) override;

			void setDevice(ofxMachineVision::DevicePtr, int deviceIndex = 0);
			shared_ptr<ofxMachineVision::Grabber::Simple> getGrabber();

			float getWidth();
			float getHeight();

			void setCalibration(cv::Mat cameraMatrix, cv::Mat distortionCoefficients);

			cv::Mat getCameraMatrix() const;
			cv::Mat getDistortionCoefficients() const;
		protected:
			void populateInspector2(ofxCvGui::ElementGroupPtr);

			void exposureCallback(float &);
			void gainCallback(float &);
			void focusCallback(float &);
			void sharpnessCallback(float &);

			shared_ptr<ofxMachineVision::Grabber::Simple> grabber;

			ofParameter<float> exposure;
			ofParameter<float> gain;
			ofParameter<float> focus;
			ofParameter<float> sharpness;

			ofParameter<float> focalLengthX, focalLengthY;
			ofParameter<float> principalPointX, principalPointY;
			ofParameter<float> distortion[4];
		};
	}
}
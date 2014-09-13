#pragma once

#include "Base.h"
#include "../../../addons/ofxMachineVision/src/ofxMachineVision.h"

#include "ofxCvMin.h"
#include "ofxRay.h"

#define OFXDIGITALEMULSION_CAMERA_DISTORTION_COEFFICIENT_COUNT 4

namespace ofxDigitalEmulsion {
	namespace Item {
		class Camera : public Base {
		public:
			Camera();
			string getTypeName() const override;
			void update() override;
			ofxCvGui::PanelPtr getView() override;

			void serialize(Json::Value &) override;
			void deserialize(const Json::Value &) override;

			void setDevice(ofxMachineVision::DevicePtr, int deviceIndex = 0);
			shared_ptr<ofxMachineVision::Grabber::Simple> getGrabber();

			float getWidth();
			float getHeight();

			void setIntrinsics(cv::Mat cameraMatrix, cv::Mat distortionCoefficients);

			cv::Mat getCameraMatrix() const;
			cv::Mat getDistortionCoefficients() const;

			const ofxRay::Camera & getRayCamera() const;
			void drawWorld() override;
		protected:
			void populateInspector2(ofxCvGui::ElementGroupPtr);
			void updateRayCamera();

			void exposureCallback(float &);
			void gainCallback(float &);
			void focusCallback(float &);
			void sharpnessCallback(float &);
			
			ofxCvGui::PanelPtr view;

			shared_ptr<ofxMachineVision::Grabber::Simple> grabber;

			ofParameter<bool> showSpecification;
			ofParameter<bool> showFocusLine;

			ofParameter<float> exposure;
			ofParameter<float> gain;
			ofParameter<float> focus;
			ofParameter<float> sharpness;

			ofParameter<float> focalLengthX, focalLengthY;
			ofParameter<float> principalPointX, principalPointY;
			ofParameter<float> distortion[4];

			ofxRay::Camera rayCamera;

			ofMesh focusLineGraph;
		};
	}
}
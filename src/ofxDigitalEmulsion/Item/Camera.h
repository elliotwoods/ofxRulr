#pragma once

#include "Base.h"
#include "../../../addons/ofxMachineVision/src/ofxMachineVision.h"
#include "../../../addons/ofxCvGui/src/ofxCvGui/Panels/Groups/Grid.h"

#include "ofxCvMin.h"
#include "ofxRay.h"

#define OFXDIGITALEMULSION_CAMERA_DISTORTION_COEFFICIENT_COUNT 4

namespace ofxDigitalEmulsion {
	namespace Item {
		class Camera : public Base {
		public:
			Camera();

			void init();
			string getTypeName() const override;
			void update();
			ofxCvGui::PanelPtr getView() override;

			void serialize(Json::Value &);
			void deserialize(const Json::Value &);

			void setDeviceIndex(int);
			void setDevice(const string & deviceTypeName);
			void setDevice(ofxMachineVision::DevicePtr);
			void clearDevice();
			shared_ptr<ofxMachineVision::Grabber::Simple> getGrabber();

			float getWidth();
			float getHeight();

			void setIntrinsics(cv::Mat cameraMatrix, cv::Mat distortionCoefficients);
			void setExtrinsics(cv::Mat rotation, cv::Mat translation);

			cv::Mat getCameraMatrix() const;
			cv::Mat getDistortionCoefficients() const;

			const ofxRay::Camera & getRayCamera() const;

			void drawWorld() override;
			ofPixels getFreshFrame();
		protected:
			void populateInspector(ofxCvGui::ElementGroupPtr);
			void updateRayCamera();

			void exposureCallback(float &);
			void gainCallback(float &);
			void focusCallback(float &);
			void sharpnessCallback(float &);
			
			shared_ptr<ofxCvGui::Panels::Groups::Grid> placeholderView;

			shared_ptr<ofxMachineVision::Grabber::Simple> grabber;

			ofParameter<int> deviceIndex;
			ofParameter<string> deviceTypeName;

			ofParameter<bool> showSpecification;
			ofParameter<bool> showFocusLine;

			ofParameter<float> exposure;
			ofParameter<float> gain;
			ofParameter<float> focus;
			ofParameter<float> sharpness;

			ofParameter<float> focalLengthX, focalLengthY;
			ofParameter<float> principalPointX, principalPointY;
			ofParameter<float> distortion[4];

			ofParameter<float> translationX, translationY, translationZ;
			ofParameter<float> rotationX, rotationY, rotationZ;

			ofxRay::Camera rayCamera;

			ofMesh focusLineGraph;
		};
	}
}
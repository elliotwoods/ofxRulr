#pragma once

#include "View.h"
#include "../../../addons/ofxMachineVision/src/ofxMachineVision.h"
#include "../../../addons/ofxCvGui/src/ofxCvGui/Panels/Groups/Grid.h"

#include "ofxCvMin.h"
#include "ofxRay.h"

#define OFXDIGITALEMULSION_CAMERA_DISTORTION_COEFFICIENT_COUNT 4

namespace ofxDigitalEmulsion {
	namespace Item {
		class Camera : public View {
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

			ofPixels getFreshFrame();
		protected:
			void populateInspector(ofxCvGui::ElementGroupPtr);
			void setAllCameraProperties();

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

			ofMesh focusLineGraph;
		};
	}
}
#pragma once

#include "View.h"
#include "ofxMachineVision.h"
#include "ofxCvGui/Panels/Groups/Grid.h"

#include "ofxCvMin.h"
#include "ofxRay.h"

#define RULR_CAMERA_DISTORTION_COEFFICIENT_COUNT 4

namespace ofxRulr {
	namespace Nodes {
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
				void reopenDevice();
				shared_ptr<ofxMachineVision::Grabber::Simple> getGrabber();

				shared_ptr<ofxMachineVision::Frame> getFreshFrame();
			protected:
				void populateInspector(ofxCvGui::InspectArguments &);
				void setAllGrabberProperties();

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
}
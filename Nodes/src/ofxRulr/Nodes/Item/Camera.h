#pragma once

#include "View.h"
#include "ofxMachineVision.h"
#include "ofxCvGui/Panels/Groups/Strip.h"

#include "ofxCvMin.h"
#include "ofxRay.h"

#define RULR_CAMERA_DISTORTION_COEFFICIENT_COUNT 4

namespace ofxRulr {
	namespace Nodes {
		namespace Item {
			class OFXRULR_API_ENTRY Camera : public View {
			public:
				Camera();
				virtual ~Camera();

				void init();
				string getTypeName() const override;
				void update();
				ofxCvGui::PanelPtr getPanel() override;

				void serialize(nlohmann::json &);
				void deserialize(const nlohmann::json &);

				void setDevice(const string & deviceTypeName);
				void setDevice(ofxMachineVision::DevicePtr, shared_ptr<ofxMachineVision::Device::Base::InitialisationSettings> = nullptr);
				void clearDevice();

				void openDevice();
				void closeDevice();

				bool isNewSingleShotFrame() const;

				shared_ptr<ofxMachineVision::Grabber::Simple> getGrabber();

				shared_ptr<ofxMachineVision::Frame> getFrame();
				shared_ptr<ofxMachineVision::Frame> getFreshFrame();
				const ofImage& getUndistortedPreview() const;

				ofxLiquidEvent<shared_ptr<ofxMachineVision::Frame>> onNewFrame;
			protected:
				void populateInspector(ofxCvGui::InspectArguments &);

				void rebuildPanel();
				void buildGrabberPanel();
				void rebuildOpenCameraPanel();

				void buildCachedInitialisationSettings();
				void applyAnyCachedInitialisationSettings(shared_ptr<ofxMachineVision::Device::Base::InitialisationSettings>);

				shared_ptr<ofxCvGui::Panels::Groups::Strip> placeholderPanel;
				shared_ptr<ofxCvGui::Panels::Widgets> cameraOpenPanel;
				shared_ptr<ofxCvGui::Panels::Draws> grabberPanel;

				shared_ptr<ofxMachineVision::Grabber::Simple> grabber; // this object should never be deallocated (otherwise other nodes will break)
				shared_ptr<ofxMachineVision::Device::Base::InitialisationSettings> initialisationSettings;
				
				nlohmann::json cachedInitialisationSettings;

				ofParameter<bool> showSpecification;
				ofParameter<bool> showFocusLine;

				ofMesh focusLineGraph;
				ofImage undistortedPreview;
			};
		}
	}
}
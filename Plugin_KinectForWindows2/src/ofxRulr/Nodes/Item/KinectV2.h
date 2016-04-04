#pragma once

#include "ofxRulr/Nodes/Item/RigidBody.h"

#include "ofxKinectForWindows2.h"

#include "ofxCvGui/Panels/Groups/Strip.h"

#ifdef  Plugin_KinectForWindows2_Exports 
	#define DLLEXPORT __declspec(dllexport)  
#else
	#define DLLEXPORT __declspec(dllimport)  
#endif

namespace ofxRulr {
	namespace Nodes {
		namespace Item {
			class DLLEXPORT KinectV2 : public ofxRulr::Nodes::Item::RigidBody {
			public:
				KinectV2();
				void init();
				string getTypeName() const override;
				void update();
				ofxCvGui::PanelPtr getPanel() override;

				void serialize(Json::Value &);
				void deserialize(const Json::Value &);

				void drawObject();
				shared_ptr<ofxKinectForWindows2::Device> getDevice();

			protected:
				void populateInspector(ofxCvGui::InspectArguments &);
				void rebuildView();

				shared_ptr<ofxKinectForWindows2::Device> device;
				shared_ptr<ofxCvGui::Panels::Groups::Strip> view;

				ofParameter<int> playState;
				ofParameter<int> viewType;

				struct {
					ofParameter<bool> rgb;
					ofParameter<bool> depth;
					ofParameter<bool> ir;
					ofParameter<bool> body;
				} enabledViews;
			};
		}
	}
}
#pragma once

#include "ofxRulr/Nodes/Procedure/Base.h"
#include "ofxCvGui/Panels/Groups/Grid.h"

namespace ofxRulr {
	namespace Nodes {
		namespace Procedure {
			namespace Calibrate {
				class CameraFromDepthCamera : public ofxRulr::Nodes::Procedure::Base {
				public:
					struct Correspondence {
						ofVec3f depthCameraObject;
						ofVec2f camera;
						ofVec2f cameraNormalized;
					};
					
					CameraFromDepthCamera();
					string getTypeName() const override;
					void init();
					ofxCvGui::PanelPtr getPanel() override;
					void update();
					
					void serialize(Json::Value &);
					void deserialize(const Json::Value &);
					
					void addCapture();
					void calibrate();
				protected:
					void populateInspector(ofxCvGui::InspectArguments &);
					void drawWorldStage();
					void rebuildView();
					shared_ptr<ofxCvGui::Panels::Groups::Grid> view;
					
					ofParameter<bool> usePreTest;
					
					vector<Correspondence> correspondences;
					vector<ofVec2f> previewCornerFindsDepthCamera;
					vector<ofVec2f> previewCornerFindsCamera;
					float error;
				};
			}
		}
	}
}
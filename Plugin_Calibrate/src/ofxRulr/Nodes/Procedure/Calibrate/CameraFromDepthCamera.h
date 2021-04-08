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
						glm::vec3 depthCameraObject;
						glm::vec2 camera;
						glm::vec2 cameraNormalized;
					};
					
					CameraFromDepthCamera();
					string getTypeName() const override;
					void init();
					ofxCvGui::PanelPtr getPanel() override;
					void update();
					
					void serialize(nlohmann::json &);
					void deserialize(const nlohmann::json &);
					
					void addCapture();
					void calibrate();
				protected:
					void populateInspector(ofxCvGui::InspectArguments &);
					void drawWorldStage();
					void rebuildView();
					shared_ptr<ofxCvGui::Panels::Groups::Grid> view;
					
					ofParameter<bool> usePreTest;
					
					vector<Correspondence> correspondences;
					vector<glm::vec2> previewCornerFindsDepthCamera;
					vector<glm::vec2> previewCornerFindsCamera;
					float error;
				};
			}
		}
	}
}
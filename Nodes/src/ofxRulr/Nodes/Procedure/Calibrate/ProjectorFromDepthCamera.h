#pragma once

#include "../Base.h"

#include "ofxCvGui/Panels/Draws.h"

namespace ofxRulr {
	namespace Nodes {
		namespace Procedure {
			namespace Calibrate {
				class ProjectorFromDepthCamera : public Base {
				public:
					struct Correspondence {
						ofVec3f world;
						ofVec2f projector;
					};
					
					ProjectorFromDepthCamera();
					string getTypeName() const override;
					ofxCvGui::PanelPtr getPanel() override;
					
					void init();
					void update();
					
					void serialize(Json::Value &);
					void deserialize(const Json::Value &);
					
					void addCapture();
					void calibrate();
					
					void drawWorld();
				protected:
					void populateInspector(ofxCvGui::InspectArguments &);
					void drawOnVideoOutput();
					
					shared_ptr<ofxCvGui::Panels::Draws> view;
					
					struct : ofParameterGroup {
						ofParameter<float> scale;
						ofParameter<float> cornersX;
						ofParameter<float> cornersY;
						ofParameter<float> positionX;
						ofParameter<float> positionY;
						ofParameter<float> brightness;
						PARAM_DECLARE("Checkerboard", scale, cornersX, cornersY, positionX, positionY, brightness);
					} checkerboard;
					
					ofParameter<float> initialLensOffset;
					
					vector<Correspondence> correspondences;
					vector<ofVec2f> previewCornerFinds;
					float error;
				};
			}
		}
	}
}
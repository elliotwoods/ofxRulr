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
						ofParameter<float> scale{ "Checkerboard Scale", 0.2f, 0.01f, 1.0f };
						ofParameter<float> cornersX{ "Checkerboard Corners X", 5, 1, 10 };
						ofParameter<float> cornersY{ "Checkerboard Corners Y", 4, 1, 10 };
						ofParameter<float> positionX{ "Checkerboard Position X", 0, -1, 1 };
						ofParameter<float> positionY{ "Checkerboard Position Y", 0, -1, 1 };
						ofParameter<float> brightness{ "Checkerboard Brightness", 0.5, 0, 1 };
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
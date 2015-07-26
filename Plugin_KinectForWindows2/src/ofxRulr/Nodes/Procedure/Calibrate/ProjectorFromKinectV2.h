#pragma once

#include "../../../addons/ofxCvGui/src/ofxCvGui/Panels/Draws.h"
#include "ofxRulr/Nodes/Procedure/Base.h"

namespace ofxRulr {
	namespace Nodes {
		namespace Procedure {
			namespace Calibrate {
				class ProjectorFromKinectV2 : public ofxRulr::Nodes::Procedure::Base {
				public:
					struct Correspondence {
						ofVec3f world;
						ofVec2f projector;
					};

					ProjectorFromKinectV2();
					string getTypeName() const override;
					void init();
					ofxCvGui::PanelPtr getView() override;
					void update();

					void serialize(Json::Value &);
					void deserialize(const Json::Value &);

					void addCapture();
					void calibrate();
				protected:
					void populateInspector(ofxCvGui::ElementGroupPtr);
					void drawWorld();
					shared_ptr<ofxCvGui::Panels::Draws> view;

					ofParameter<float> checkerboardScale;
					ofParameter<float> checkerboardCornersX;
					ofParameter<float> checkerboardCornersY;
					ofParameter<float> checkerboardPositionX;
					ofParameter<float> checkerboardPositionY;
					ofParameter<float> checkerboardBrightness;

					ofParameter<float> initialLensOffset;
					ofParameter<bool> trimOutliers;

					vector<Correspondence> correspondences;
					vector<ofVec2f> previewCornerFinds;
					float error;
				};
			}
		}
	}
}
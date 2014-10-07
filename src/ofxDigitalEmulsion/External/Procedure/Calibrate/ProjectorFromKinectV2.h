#pragma once

#include "../../../Procedure/Base.h"

namespace ofxDigitalEmulsion {
	namespace Procedure {
		namespace Calibrate {
			class ProjectorFromKinectV2 : public ofxDigitalEmulsion::Procedure::Base {
			public:
				struct Correspondence {
					ofVec3f world;
					ofVec2f projector;
				};

				ProjectorFromKinectV2();
				void init() override;
				string getTypeName() const override;
				ofxCvGui::PanelPtr getView() override;
				void update() override;

				void serialize(Json::Value &) override;
				void deserialize(const Json::Value &) override;

				void addCapture();
				void calibrate();
			protected:
				void populateInspector2(ofxCvGui::ElementGroupPtr) override;
				void drawWorld();
				ofxCvGui::PanelPtr view;

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
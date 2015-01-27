#pragma once

#include "../../../Procedure/Base.h"

namespace ofxDigitalEmulsion {
	namespace Procedure {
		namespace Calibrate {
			class CameraFromKinectV2 : public ofxDigitalEmulsion::Procedure::Base {
			public:
				struct Correspondence {
					ofVec3f world;
					ofVec2f camera;
				};

				CameraFromKinectV2();
				void init() override;
				string getTypeName() const override;
				ofxCvGui::PanelPtr getView() override;
				void update() override;

				void serialize(Json::Value &);
				void deserialize(const Json::Value &);

				void addCapture();
				void calibrate();
			protected:
				void populateInspector(ofxCvGui::ElementGroupPtr);
				void drawWorld();
				ofxCvGui::PanelPtr view;

				ofParameter<bool> usePreTest;

				vector<Correspondence> correspondences;
				vector<ofVec2f> previewCornerFindsKinect;
				vector<ofVec2f> previewCornerFindsCamera;
				float error;
			};
		}
	}
}
#pragma once

#include "../Base.h"
#include "ofxCvMin.h"

namespace ofxDigitalEmulsion {
	namespace Procedure {
		namespace Calibrate {
			class CameraIntrinsics : public Base {
			public:
				CameraIntrinsics();
				void init();
				string getTypeName() const override;
				ofxCvGui::PanelPtr getView() override;
				void update();

				void serialize(Json::Value &);
				void deserialize(const Json::Value &);
			protected:
				void populateInspector(ofxCvGui::ElementGroupPtr);
				void calibrate();

				ofxCvGui::PanelPtr view;
				ofParameter<bool> enableFinder;
				ofImage grayscale;

				vector<ofVec2f> currentCorners;
				vector<vector<ofVec2f>> accumulatedCorners;
				ofParameter<float> error;
			};
		}
	}
}
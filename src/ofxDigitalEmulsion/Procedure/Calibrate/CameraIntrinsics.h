#pragma once

#include "../Base.h"
#include "ofxCvMin.h"

namespace ofxDigitalEmulsion {
	namespace Procedure {
		namespace Calibrate {
			class CameraIntrinsics : public Base {
			public:
				CameraIntrinsics();
				string getTypeName() const override;
				Graph::PinSet getInputPins() override;
				void populateInspector(ofxCvGui::ElementGroupPtr) override;
				ofxCvGui::PanelPtr getView() override;
				void update() override;
				void calibrate();
			protected:
				Graph::PinSet inputPins;
				ofParameter<bool> enableFinder;
				ofImage grayscale;

				vector<ofVec2f> currentCorners;
				vector<vector<ofVec2f>> accumulatedCorners;
			};
		}
	}
}
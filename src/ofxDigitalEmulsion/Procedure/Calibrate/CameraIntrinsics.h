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
				Graph::PinSet getInputPins() const override;
				ofxCvGui::PanelPtr getView() override;
				void update() override;

				void serialize(Json::Value &);
				void deserialize(const Json::Value &);
			protected:
				void populateInspector2(ofxCvGui::ElementGroupPtr) override;
				void calibrate();

				Graph::PinSet inputPins;
				ofParameter<bool> enableFinder;
				ofImage grayscale;

				vector<ofVec2f> currentCorners;
				vector<vector<ofVec2f>> accumulatedCorners;
				float error;
			};
		}
	}
}
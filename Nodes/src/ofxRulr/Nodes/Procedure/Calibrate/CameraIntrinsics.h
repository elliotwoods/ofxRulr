#pragma once

#include "../Base.h"
#include "ofxCvMin.h"

namespace ofxRulr {
	namespace Nodes {
		namespace Procedure {
			namespace Calibrate {
				class CameraIntrinsics : public Base {
				public:
					CameraIntrinsics();
					void init();
					string getTypeName() const override;
					ofxCvGui::PanelPtr getPanel() override;
					void update();

					void serialize(Json::Value &);
					void deserialize(const Json::Value &);

					bool getRunFinderEnabled() const;
				protected:
					void populateInspector(ofxCvGui::InspectArguments &);
					void addBoard(bool tetheredCapture);
					void findBoard();
					void calibrate();

					ofxCvGui::PanelPtr view;
					ofImage grayscale;

					vector<ofVec2f> currentCorners;
					vector<vector<ofVec2f>> accumulatedCorners;
					ofParameter<float> error;
					ofParameter<bool> tetheredShootMode{ "Tethered shoot mode", false };
				};
			}
		}
	}
}
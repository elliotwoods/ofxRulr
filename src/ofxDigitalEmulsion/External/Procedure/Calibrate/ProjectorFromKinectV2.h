#pragma once

#include "../../../Procedure/Base.h"

namespace ofxDigitalEmulsion {
	namespace Procedure {
		namespace Calibrate {
			class ProjectorFromKinectV2 : public ofxDigitalEmulsion::Procedure::Base {
			public:
				ProjectorFromKinectV2();
				string getTypeName() const override;
				Graph::PinSet getInputPins() const override;
				ofxCvGui::PanelPtr getView() override;
				void update() override;

				void serialize(Json::Value &) override;
				void deserialize(const Json::Value &) override;

				void addCapture();
			protected:
				void populateInspector2(ofxCvGui::ElementGroupPtr) override;
				void drawWorld();
				Graph::PinSet inputPins;
				ofxCvGui::PanelPtr view;

				ofParameter<float> checkerboardScale;
				ofParameter<float> checkerboardCornersX;
				ofParameter<float> checkerboardCornersY;
				ofParameter<float> checkerboardPositionX;
				ofParameter<float> checkerboardPositionY;
			};
		}
	}
}
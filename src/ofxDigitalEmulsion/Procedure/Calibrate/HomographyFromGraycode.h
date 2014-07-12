#pragma once

#include "../Base.h"

namespace ofxDigitalEmulsion {
	namespace Procedure {
		namespace Calibrate {
			class HomographyFromGraycode : public Procedure::Base {
			public:
				HomographyFromGraycode();
				string getTypeName() const override;
				Graph::PinSet getInputPins() const override;
				ofxCvGui::PanelPtr getView() override;

				void serialize(Json::Value &) override;
				void deserialize(const Json::Value &) override;

			protected:
				void populateInspector2(ofxCvGui::ElementGroupPtr) override;
				void fit();

				Graph::PinSet inputPins;
				ofxCvGui::PanelPtr view;

				ofMatrix4x4 cameraToProjector;
				ofMesh grid;
			};
		}
	}
}
#pragma once

#include "../Base.h"

#include "ofxCvGui/Panels/Image.h"

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
				void update() override;
			protected:
				void populateInspector2(ofxCvGui::ElementGroupPtr) override;
				void findHomography();
				void findDistortionCoefficients();
				void saveMappingImage() const;

				Graph::PinSet inputPins;
				shared_ptr<ofxCvGui::Panels::Image> view;

				ofMatrix4x4 cameraToProjector;
				ofMesh grid;
				ofImage dummy;
			};
		}
	}
}
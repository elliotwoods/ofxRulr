#pragma once

#include "../Base.h"

#include "ofxCvGui/Panels/Image.h"

namespace ofxDigitalEmulsion {
	namespace Procedure {
		namespace Calibrate {
			class HomographyFromGraycode : public Procedure::Base {
			public:
				HomographyFromGraycode();
				void init() override;
				string getTypeName() const override;
				ofxCvGui::PanelPtr getView() override;

				void serialize(Json::Value &) override;
				void deserialize(const Json::Value &) override;
				void update() override;
				
				void findHomography();
				void findDistortionCoefficients();
				void exportMappingImage(string filename = "") const;
			protected:
				void populateInspector2(ofxCvGui::ElementGroupPtr) override;

				shared_ptr<ofxCvGui::Panels::Image> view;

				ofMatrix4x4 cameraToProjector;
				ofMesh grid;
				ofImage dummy;

				ofParameter<bool> doubleExportSize;
			};
		}
	}
}
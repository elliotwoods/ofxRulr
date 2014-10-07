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
				ofxCvGui::PanelPtr getView() override;

				void serialize(Json::Value &) override;
				void deserialize(const Json::Value &) override;
				void update() override;
				
				void findHomography();
				void findDistortionCoefficients();
<<<<<<< HEAD
				void saveMappingImage() const;
=======
				void exportMappingImage(string filename = "") const;
			protected:
				void populateInspector2(ofxCvGui::ElementGroupPtr) override;
>>>>>>> 8ebc7c72b77d159e8426e946619748203d7b3608

				shared_ptr<ofxCvGui::Panels::Image> view;

				ofMatrix4x4 cameraToProjector;
				ofMesh grid;
				ofImage dummy;

				ofParameter<bool> doubleExportSize;
			};
		}
	}
}
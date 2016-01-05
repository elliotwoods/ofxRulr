#pragma once

#include "../Base.h"

#include "ofxCvGui/Panels/Image.h"

namespace ofxRulr {
	namespace Nodes {
		namespace Procedure {
			namespace Calibrate {
				class HomographyFromGraycode : public Procedure::Base {
				public:
					HomographyFromGraycode();
					void init();
					string getTypeName() const override;
					ofxCvGui::PanelPtr getView() override;
					void update();

					void serialize(Json::Value &);
					void deserialize(const Json::Value &);

					void findHomography();
					void findDistortionCoefficients();
					void exportMappingImage(string filename = "") const;
				protected:
					void populateInspector(ofxCvGui::InspectArguments &);

					shared_ptr<ofxCvGui::Panels::Image> view;

					ofMatrix4x4 cameraToProjector;
					ofMesh grid;
					ofImage dummy;

					ofParameter<bool> undistortFirst;
					ofParameter<bool> doubleExportSize;
				};
			}
		}
	}
}
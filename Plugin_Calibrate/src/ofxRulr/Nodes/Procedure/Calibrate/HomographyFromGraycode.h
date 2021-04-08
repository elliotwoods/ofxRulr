#pragma once

#include "ofxRulr.h"

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
					ofxCvGui::PanelPtr getPanel() override;
					void update();

					void serialize(nlohmann::json &);
					void deserialize(const nlohmann::json &);

					void findHomography();
					void findDistortionCoefficients();
					void exportMappingImage(string filename = "") const;
				protected:
					void populateInspector(ofxCvGui::InspectArguments &);

					shared_ptr<ofxCvGui::Panels::Image> view;

					glm::mat4 cameraToProjector;
					ofMesh grid;
					ofImage dummy;

					ofParameter<bool> undistortFirst;
					ofParameter<bool> doubleExportSize;
				};
			}
		}
	}
}
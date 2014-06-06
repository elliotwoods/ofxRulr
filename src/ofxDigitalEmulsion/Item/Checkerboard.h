#pragma once

#include "Base.h"
#include "ofxCvMin.h"

namespace ofxDigitalEmulsion {
	namespace Item {
		class Checkerboard : public Base {
		public:
			Checkerboard();
			string getTypeName() const override;
			void populateInspector(ofxCvGui::ElementGroupPtr) override;
			ofxCvGui::PanelPtr getView();

			cv::Size getSize();
		protected:
			void updatePreviewMesh();

			ofParameter<float> sizeX, sizeY;
			ofParameter<float> spacing;
			ofMesh previewMesh;
		};
	}
}
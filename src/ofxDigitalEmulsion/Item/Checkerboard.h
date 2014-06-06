#pragma once

#include "Base.h"
#include "ofxCvMin.h"

namespace ofxDigitalEmulsion {
	namespace Item {
		class Checkerboard : public Base {
		public:
			Checkerboard();
			string getTypeName() const override;
			void populate(ofxCvGui::ElementGroupPtr) override;
			ofxCvGui::PanelPtr getView();

			cv::Size getSize();
		protected:
			ofParameter<float> sizeX, sizeY;
			ofParameter<float> spacing;
		};
	}
}
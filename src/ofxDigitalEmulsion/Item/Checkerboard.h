#pragma once

#include "Base.h"

namespace ofxDigitalEmulsion {
	namespace Item {
		class Checkerboard : public Base {
		public:
			Checkerboard();
			string getTypeName() const override;
			void populate(ofxCvGui::ElementGroupPtr) override;
		protected:
			ofParameter<float> sizeX, sizeY;
			ofParameter<float> spacing;
		};
	}
}
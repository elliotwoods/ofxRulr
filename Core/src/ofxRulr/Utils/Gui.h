#pragma once

#include "ofxCvGui.h"
#include "ofxRulr/Utils/Constants.h"

namespace ofxRulr {
	namespace Utils {
		namespace Gui {
			OFXRULR_API_ENTRY shared_ptr<ofxCvGui::Widgets::Slider> addIntSlider(ofParameter<float> & parameter, ofxCvGui::ElementGroupPtr inspector);
		}
	}
}
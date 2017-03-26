#pragma once

#include "ofxCvGui.h"
#include "ofxRulr/Utils/Constants.h"

namespace ofxRulr {
	namespace Utils {
		namespace Gui {
			RULR_EXPORTS shared_ptr<ofxCvGui::Widgets::Slider> addIntSlider(ofParameter<float> & parameter, ofxCvGui::ElementGroupPtr inspector);
		}
	}
}
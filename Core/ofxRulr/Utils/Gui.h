#pragma once

#include "ofxCvGui.h"

namespace ofxRulr {
	namespace Utils {
		namespace Gui {
			shared_ptr<ofxCvGui::Widgets::Slider> addIntSlider(ofParameter<float> & parameter, ofxCvGui::ElementGroupPtr inspector);
		}
	}
}
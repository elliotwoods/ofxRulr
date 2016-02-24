#include "pch_RulrCore.h"
#include "Gui.h"

using namespace ofxCvGui;

namespace ofxRulr {
	namespace Utils {
		namespace Gui {
			shared_ptr<ofxCvGui::Widgets::Slider> addIntSlider(ofParameter<float> & parameter, ElementGroupPtr inspector) {
				auto slider = make_shared<Widgets::Slider>(parameter);
				slider->addIntValidator();
				inspector->add(slider);
				return slider;
			}
		}
	}
}
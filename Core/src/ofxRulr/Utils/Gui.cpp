#include "Gui.h"

using namespace ofxCvGui;

namespace ofxRulr {
	namespace Utils {
		namespace Gui {
			shared_ptr<ofxCvGui::Widgets::Slider> addIntSlider(ofParameter<float> & parameter, ElementGroupPtr inspector) {
				auto slider = Widgets::Slider::make(parameter);
				slider->addIntValidator();
				inspector->add(slider);
				return slider;
			}
		}
	}
}
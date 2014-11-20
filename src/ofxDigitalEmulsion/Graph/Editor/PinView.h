#pragma once

#include "ofxCvGui/Element.h"

namespace ofxDigitalEmulsion {
	namespace Graph {
		namespace Editor {
			class PinView : public ofxCvGui::Element {
			public:
				PinView(string nodeTypeName);
				void setTypeName(string nodeTypeName);
			protected:
				string nodeTypeName;
			};
		}
	}
}
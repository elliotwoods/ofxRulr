#pragma once

#include "NodeHost.h"

namespace ofxDigitalEmulsion {
	namespace Graph {
		namespace Editor {
			class LinkHost : public ofxCvGui::Element {
			public:
				typedef unsigned int Index;
			protected:
				weak_ptr<NodeHost> sourceNode;
				weak_ptr<NodeHost> targetNode;
				weak_ptr<BasePin> targetPin;
			};
		}
	}
}

#pragma once

#include "ofxCvGui/Element.h"

namespace ofxDigitalEmulsion {
	namespace Graph {
		class Node;

		namespace Editor {

			class PinView : public ofxCvGui::Element {
			public:
				PinView();

				void setup(Node & node);

				template<typename NodeType>
				void setup() {
					NodeType tempNode;
					this->setup(tempNode);
				}
			protected:
				ofImage * icon;
				string nodeTypeName;
			};
		}
	}
}
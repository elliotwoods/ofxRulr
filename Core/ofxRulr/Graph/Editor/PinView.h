#pragma once

#include "ofxCvGui/Element.h"

namespace ofxRulr {
	namespace Graph {
		class Node;

		namespace Editor {

			/// A class to draw a preview icon for a Node type
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
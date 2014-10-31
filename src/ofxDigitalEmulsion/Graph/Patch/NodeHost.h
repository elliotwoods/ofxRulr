#pragma once

#include "ofxDigitalEmulsion/Graph/Node.h"

namespace ofxDigitalEmulsion {
	namespace Graph {
		namespace Patch {
			/***
			Possesses the node
			*/
			class NodeHost : public ofxCvGui::Element {
			public:
				typedef unsigned int Index;

				NodeHost(shared_ptr<Node>);
				shared_ptr<Node> getNodeInstance();
			protected:
				shared_ptr<Node> node;
			};
		}
	}
}
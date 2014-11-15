#pragma once

#include "ofxDigitalEmulsion/Graph/Node.h"

#include "ofxCvGui/Utils/Button.h"

namespace ofxDigitalEmulsion {
	namespace Graph {
		namespace Editor {
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

				ofxCvGui::PanelPtr nodeView;
				ofxCvGui::ElementGroupPtr elements;
				ofxCvGui::ElementGroupPtr inputPins;
			};
		}
	}
}
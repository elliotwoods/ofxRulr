#pragma once

#include "PinView.h"
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
				ofVec2f getInputPinPosition(shared_ptr<AbstractPin>) const;
				ofVec2f getOutputPinPositionGlobal() const;

				ofxLiquidEvent<const shared_ptr<AbstractPin>> onBeginMakeConnection;
				ofxLiquidEvent<ofxCvGui::MouseArguments> onReleaseMakeConnection;
				ofxLiquidEvent<const shared_ptr<AbstractPin>> onDropInputConnection;

				void serialize(Json::Value &);

			protected:
				ofVec2f getOutputPinPosition() const;
				shared_ptr<Node> node;
				ofVec2f outputPinPosition;

				ofxCvGui::PanelPtr nodeView;
				ofxCvGui::ElementGroupPtr elements;
				ofxCvGui::ElementGroupPtr inputPins;
			};
		}
	}
}
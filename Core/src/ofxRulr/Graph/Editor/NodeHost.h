#pragma once

#include "PinView.h"
#include "ofxRulr/Nodes/Base.h"
#include "ofxCvGui/Utils/Button.h"

#define RULR_NODEHOST_INPUTAREA_WIDTH 85
#define RULR_NODEHOST_TITLE_WIDTH 50
#define RULR_NODEHOST_OUTPUTAREA_WIDTH 100

namespace ofxRulr {
	namespace Graph {
		namespace Editor {
			/***
			Possesses the node
			*/
			class NodeHost : public ofxCvGui::Element {
			public:
				typedef unsigned int Index;

				NodeHost(shared_ptr<Nodes::Base>);
				shared_ptr<Nodes::Base> getNodeInstance();
				ofVec2f getInputPinPosition(shared_ptr<AbstractPin>) const;
				ofVec2f getOutputPinPositionGlobal() const;

				ofxLiquidEvent<const shared_ptr<AbstractPin>> onBeginMakeConnection;
				ofxLiquidEvent<ofxCvGui::MouseArguments> onReleaseMakeConnection;
				ofxLiquidEvent<const shared_ptr<AbstractPin>> onDropInputConnection;

				void serialize(Json::Value &);

			protected:
				void rebuildInputPins();
				ofVec2f getOutputPinPosition() const;
				shared_ptr<Nodes::Base> node;
				ofVec2f outputPinPosition;

				ofxCvGui::PanelPtr nodeView;
				ofxCvGui::ElementGroupPtr elements;
				ofxCvGui::ElementGroupPtr inputPins;
			};
		}
	}
}
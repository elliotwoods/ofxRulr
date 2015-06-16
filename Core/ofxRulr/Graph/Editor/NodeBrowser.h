#pragma once

#include "../Factory.h"
#include "ofxCvGui/Element.h"
#include "ofxCvGui/Utils/TextField.h"
#include "ofxCvGui/Panels/Scroll.h"

namespace ofxRulr {
	namespace Graph {
		namespace Editor {
			class NodeBrowser : public ofxCvGui::Element {
				class ListItem : public ofxCvGui::Element {
				public:
					ListItem(shared_ptr<BaseFactory>);
					shared_ptr<BaseFactory> getFactory();
				protected:
					shared_ptr<BaseFactory> factory;
				};

			public:
				NodeBrowser();
				void reset();
				void refreshResults();
				ofxLiquidEvent<shared_ptr<Node>> onNewNode;
			protected:
				void buildBackground();
				void buildDialog();
				void buildTextBox();
				void buildListBox();

				void notifyNewNode();

				ofxCvGui::ElementPtr background;
				ofxCvGui::ElementGroupPtr dialog;
				shared_ptr<ofxCvGui::Utils::TextField> textBox;
				shared_ptr<ofxCvGui::Panels::Scroll> listBox;

				weak_ptr<ListItem> currentSelection;
				ofVec2f mouseDownInListPosition;
			};
		}
	}
}
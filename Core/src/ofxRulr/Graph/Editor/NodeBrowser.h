#pragma once

#include "../FactoryRegister.h"
#include "ofxCvGui/Element.h"
#include "ofxCvGui/Utils/TextField.h"
#include "ofxCvGui/Panels/Scroll.h"

namespace ofxRulr {
	namespace Graph {
		namespace Editor {
			class NodeBrowser : public ofxCvGui::Panels::Base {
				class ListItem : public ofxCvGui::Element {
				public:
					ListItem(shared_ptr<BaseFactory>);
					shared_ptr<BaseFactory> getFactory();
					const ofImage & getIcon();
				protected:
					shared_ptr<BaseFactory> factory;
					shared_ptr<ofImage> icon;
				};

			public:
				NodeBrowser();
				void reset();
				void refreshResults();
				ofxLiquidEvent<shared_ptr<Nodes::Base>> onNewNode;
			protected:
				void build();
				void buildTextBox();
				void buildListBox();

				void notifyNewNode();

				shared_ptr<ofxCvGui::Utils::TextField> textBox;
				shared_ptr<ofxCvGui::Panels::Scroll> listBox;

				weak_ptr<ListItem> currentSelection;
				ofVec2f mouseDownInListPosition;
			};
		}
	}
}
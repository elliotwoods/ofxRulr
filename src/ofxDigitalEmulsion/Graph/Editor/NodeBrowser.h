#pragma once

#include "../Factory.h"
#include "ofxCvGui/Element.h"
#include "ofxCvGui/Utils/TextField.h"
#include "ofxCvGui/Panels/Scroll.h"
#include "ofxTextInputField.h"

namespace ofxDigitalEmulsion {
	namespace Graph {
		namespace Editor {
			class NodeBrowser : public ofxCvGui::Element {
				class ListItem : public ofxCvGui::Element {
				public:
					ListItem(shared_ptr<BaseFactory>);
				};

			public:
				NodeBrowser();
				void setBirthLocation(const ofVec2f &);
				void reset();
				void refreshResults();
			protected:
				void buildBackground();

				void buildDialog();
				void buildTextBox();
				void buildListBox();

				ofVec2f birthLocation;

				ofxCvGui::ElementPtr background;
				ofxCvGui::ElementGroupPtr dialog;
				shared_ptr<ofxCvGui::Utils::TextField> textBox;
				shared_ptr<ofxCvGui::Panels::Scroll> listBox;
			};
		}
	}
}
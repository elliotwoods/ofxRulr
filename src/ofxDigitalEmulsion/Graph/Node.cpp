#include "Node.h"

#include "ofxCvGui.h"

using namespace ofxCvGui;

namespace ofxDigitalEmulsion {
	namespace Graph {
		//----------
		PinSet Node::getInputPins() {
			return PinSet();
		}

		//----------
		void Node::populateInspector(ofxCvGui::ElementGroupPtr inspector) {
			inspector->add(Widgets::Title::make(this->getTypeName(), ofxCvGui::Widgets::Title::Level::H2));
			inspector->add(Widgets::Button::make("Save", [this] () {
				this->save(this->getDefaultFilename());
			}));
			inspector->add(Widgets::Button::make("Load", [this] () {
				this->load(this->getDefaultFilename());
			}));
			inspector->add(Widgets::Spacer::make());

			this->populateInspector2(inspector);
		}
	}
}
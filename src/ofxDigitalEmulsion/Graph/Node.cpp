#include "Node.h"

#include "../Utils/Exception.h"

#include "ofxCvGui.h"

using namespace ofxCvGui;

namespace ofxDigitalEmulsion {
	namespace Graph {
		//----------
		PinSet Node::getInputPins() const {
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
//cancel this out in vs2012 until system dialogs are fixed on vs2012
#ifndef TARGET_WIN32
			inspector->add(Widgets::Button::make("Load from...", [this] () {
				this->load();
			}));
#endif
			inspector->add(Widgets::Spacer::make());

			this->populateInspector2(inspector);
		}

		//----------
		void Node::throwIfMissingAConnection() const {
			const auto inputPins = this->getInputPins();
			for(auto & inputPin : inputPins) {
				if (!inputPin->isConnected()) {
					stringstream message;
					message << "Node [" << this->getTypeName() << "] is missing connection [" << inputPin->getName() << "]";
					throw(Utils::Exception(message.str()));
				}
			}
		}
	}
}
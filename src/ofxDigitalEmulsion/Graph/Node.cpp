#include "Node.h"

#include "../Utils/Exception.h"

#include "ofxCvGui.h"

using namespace ofxCvGui;

namespace ofxDigitalEmulsion {
	namespace Graph {
		//----------
		Node::Node() {
			this->icon = 0;
			this->defaultIconName = "Default";
		}

		//----------
		string Node::getName() const {
			if (this->name.empty()) {
				return this->getTypeName();
			} else {
				return this->name;
			}
		}

		//----------
		void Node::setName(const string name) {
			this->name = name;
			auto view = this->getView();
			if (view) {
				view->setCaption(this->name);
			}
		}

		//----------
		ofImage & Node::getIcon() {
			this->setupIcon();
			return * this->icon;
		}

		//----------
		const PinSet & Node::getInputPins() const {
			return this->inputPins;
		}

		//----------
		void Node::populateInspector(ofxCvGui::ElementGroupPtr inspector) {
			inspector->add(Widgets::Title::make(this->getTypeName(), ofxCvGui::Widgets::Title::Level::H2));
			inspector->add(Widgets::Button::make("Save Node...", [this] () {
				try {
					auto result = ofSystemSaveDialog(this->getDefaultFilename(), "Save node [" + this->getName() + "] as json");
					if (result.bSuccess) {
						this->save(result.getPath());
					}
				}
				OFXDIGITALEMULSION_CATCH_ALL_TO_ALERT
			}));
			inspector->add(Widgets::Button::make("Load Node...", [this] () {
				try {
					auto result = ofSystemLoadDialog("Load node [" + this->getName() + "] from json");
					if (result.bSuccess) {
						this->load(result.getPath());
					}
				}
				OFXDIGITALEMULSION_CATCH_ALL_TO_ALERT
			}));
//cancel this out in vs2012 until system dialogs are fixed on vs2012
#ifndef TARGET_WIN32
			inspector->add(Widgets::Button::make("Load from...", [this] () {
				try {
					this->load();
				}
				OFXDIGITALEMULSION_CATCH_ALL_TO_ALERT
			}));
#endif
			for (auto inputPin : this->getInputPins()) {
				inspector->add(Widgets::Indicator::make(inputPin->getName(), [inputPin]() {
					return (Widgets::Indicator::Status) inputPin->isConnected();
				}));
			}
			inspector->add(Widgets::Spacer::make());

			this->populateInspector2(inspector);
		}

		//----------
		void Node::throwIfMissingAnyConnection() const {
			const auto inputPins = this->getInputPins();
			for(auto & inputPin : inputPins) {
				if (!inputPin->isConnected()) {
					stringstream message;
					message << "Node [" << this->getTypeName() << "] is missing connection [" << inputPin->getName() << "]";
					throw(Utils::Exception(message.str()));
				}
			}
		}

		//----------
		void Node::addInput(shared_ptr<BasePin> pin) {
			this->inputPins.add(pin);
		}

		//----------
		void Node::removeInput(shared_ptr<BasePin> pin) {
			this->inputPins.remove(pin);
		}

		//----------
		void Node::clearInputs() {
			this->inputPins.clear();
		}

		//----------
		void Node::setupIcon() {
			if (this->icon) {
				//icon is already setup
				return;
			}

			const auto imageName = "ofxDigitalEmulsion::Nodes::" + this->getTypeName();
			if (ofxAssets::AssetRegister.hasImage(imageName)) {
				//setup with specific node icon
				this->icon = &ofxAssets::image(imageName);
			}
			else {
				//setup with inherited node icon
				this->icon = &ofxAssets::image("ofxDigitalEmulsion::Nodes::" + this->defaultIconName);
			}
		}
	}
}
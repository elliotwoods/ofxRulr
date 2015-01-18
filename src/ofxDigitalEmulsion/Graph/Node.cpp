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
			this->setupGraphics();
			return * this->icon;
		}

		//----------
		const ofColor & Node::getColor() {
			this->setupGraphics();
			return this->color;
		}

		//----------
		const PinSet & Node::getInputPins() const {
			return this->inputPins;
		}

		//----------
		void Node::populateInspector(ofxCvGui::ElementGroupPtr inspector) {
			auto nameWidget = Widgets::Title::make(this->getName(), ofxCvGui::Widgets::Title::Level::H1);
			auto nameWidgetWeak = weak_ptr<Element>(nameWidget);
			nameWidget->onDraw += [this](ofxCvGui::DrawArguments & args) {
				ofxAssets::image("ofxCvGui::edit").draw(ofRectangle(args.localBounds.width - 20, 5, 15, 15));
			};
			nameWidget->onMouseReleased += [this, nameWidgetWeak](ofxCvGui::MouseArguments & args) {
				auto nameWidget = nameWidgetWeak.lock();
				if (nameWidget) {
					auto result = ofSystemTextBoxDialog("Change name of [" + this->getTypeName() + "] node (" + this->getName() + ")");
					if (result != "") {
						this->setName(result);
					nameWidget->setCaption(result);
					}
				}
			};
			inspector->add(nameWidget);

			inspector->add(Widgets::Title::make(this->getTypeName(), ofxCvGui::Widgets::Title::Level::H3));

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

			//pin status
			for (auto inputPin : this->getInputPins()) {
				inspector->add(Widgets::Indicator::make(inputPin->getName(), [inputPin]() {
					return (Widgets::Indicator::Status) inputPin->isConnected();
				}));
			}

			//node parameters
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
		void Node::addInput(shared_ptr<AbstractPin> pin) {
			this->inputPins.add(pin);
		}

		//----------
		void Node::removeInput(shared_ptr<AbstractPin> pin) {
			this->inputPins.remove(pin);
		}

		//----------
		void Node::clearInputs() {
			this->inputPins.clear();
		}

		//----------
		void Node::setupGraphics() {
			if (this->icon) {
				//icon is already setup, we presume all graphics are already setup
				return;
			}

			//setup icon
			const auto imageName = "ofxDigitalEmulsion::Nodes::" + this->getTypeName();
			if (ofxAssets::AssetRegister.hasImage(imageName)) {
				//setup with specific node icon
				this->icon = &ofxAssets::image(imageName);
			}
			else {
				//setup with inherited node icon
				this->icon = &ofxAssets::image("ofxDigitalEmulsion::Nodes::" + this->defaultIconName);
			}

			//setup color
			auto hash = std::hash<string>()(this->getTypeName());
			hash %= 255;
			auto color = ofColor(200, 100, 100);
			color.setHue((hash % 64) * (256 / 64));
			color.setBrightness((hash / 64) * 64 + 128);
			this->color = color;
		}

		//----------
		void Node::setIcon(ofImage & icon) {
			this->icon = &icon;
		}
	}
}
#include "Base.h"

#include "../Exception.h"

#include "ofxCvGui.h"

using namespace ofxCvGui;

namespace ofxRulr {
	namespace Nodes {
		//----------
		Base::Base() {
			this->icon = 0;
			this->initialized = false;
			this->defaultIconName = "Default";
		}

		//----------
		Base::~Base() {
			if (this->initialized) {
				//pins will try to notify this node when connections are dropped, so drop the pins first
				for (auto pin : this->inputPins) {
					pin->resetConnection();
				}
				this->onDestroy.notifyListenersInReverse();
				this->initialized = false;
			}
		}

		//----------
		string Base::getTypeName() const {
			return "Node";
		}

		//----------
		void Base::init() {
			this->onInspect.addListener([this](ofxCvGui::InspectArguments & args) {
				this->populateInspector(args.inspector);
			}, 99999, this); // populate the instpector with this at the top. We call notify in reverse for inheritance

			//notify the subclasses to init
			this->onInit.notifyListeners();
			this->initialized = true;
		}

		//----------
		void Base::update() {
			this->onUpdate.notifyListeners();
		}

		//----------
		string Base::getName() const {
			if (this->name.empty()) {
				return this->getTypeName();
			} else {
				return this->name;
			}
		}

		//----------
		void Base::setName(const string name) {
			this->name = name;
		}

		//----------
		ofImage & Base::getIcon() {
			this->setupGraphics();
			return * this->icon;
		}

		//----------
		const ofColor & Base::getColor() {
			this->setupGraphics();
			return this->color;
		}

		//----------
		const Graph::PinSet & Base::getInputPins() const {
			return this->inputPins;
		}

		//----------
		void Base::populateInspector(ofxCvGui::ElementGroupPtr inspector) {
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
				RULR_CATCH_ALL_TO_ALERT
			}));

			inspector->add(Widgets::Button::make("Load Node...", [this] () {
				try {
					auto result = ofSystemLoadDialog("Load node [" + this->getName() + "] from json");
					if (result.bSuccess) {
						this->load(result.getPath());
					}
				}
				RULR_CATCH_ALL_TO_ALERT
			}));

			//pin status
			for (auto inputPin : this->getInputPins()) {
				inspector->add(Widgets::Indicator::make(inputPin->getName(), [inputPin]() {
					return (Widgets::Indicator::Status) inputPin->isConnected();
				}));
			}

			//node parameters
			inspector->add(Widgets::Spacer::make());
		}

		//----------
		void Base::throwIfMissingAnyConnection() const {
			const auto inputPins = this->getInputPins();
			for(auto & inputPin : inputPins) {
				if (!inputPin->isConnected()) {
					stringstream message;
					message << "Node [" << this->getTypeName() << "] is missing connection [" << inputPin->getName() << "]";
					throw(Exception(message.str()));
				}
			}
		}

		//----------
		void Base::addInput(shared_ptr<Graph::AbstractPin> pin) {
			//setup events to fire on this node for this pin
			auto pinWeak = weak_ptr<Graph::AbstractPin>(pin);
			pin->onNewConnectionUntyped += [this, pinWeak](shared_ptr<Base> &) {
				auto pin = pinWeak.lock();
				if (pin) {
					this->onConnect(pin);
				}
				this->onAnyInputConnectionChanged.notifyListeners();
			};
			pin->onDeleteConnectionUntyped += [this, pinWeak](shared_ptr<Base> &) {
				auto pin = pinWeak.lock();
				if (pin) {
					this->onDisconnect(pin);
				}
				this->onAnyInputConnectionChanged.notifyListeners();
			};

			this->inputPins.add(pin);
		}

		//----------
		void Base::removeInput(shared_ptr<Graph::AbstractPin> pin) {
			this->inputPins.remove(pin);
		}

		//----------
		void Base::clearInputs() {
			this->inputPins.clear();
		}

		//----------
		void Base::setupGraphics() {
			if (this->icon) {
				//icon is already setup, we presume all graphics are already setup
				return;
			}

			//setup icon
			const auto imageName = "ofxRulr::Nodes::" + this->getTypeName();
			if (ofxAssets::Register::X().hasImage(imageName)) {
				//setup with specific node icon
				this->icon = &ofxAssets::image(imageName);
			}
			else {
				//setup with inherited node icon
				this->icon = & ofxAssets::image("ofxRulr::Nodes::" + this->defaultIconName);
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
		void Base::setIcon(ofImage & icon) {
			this->icon = &icon;
		}
	}
}
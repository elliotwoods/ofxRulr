#include "pch_RulrCore.h"
#include "Base.h"

#include "ofxRulr/Graph/Editor/NodeHost.h"
#include "ofxRulr/Graph/World.h"
#include "../Exception.h"
#include "GraphicsManager.h"

using namespace ofxCvGui;

namespace ofxRulr {
	namespace Nodes {
		//----------
		Base::Base() {
			this->initialized = false;
			this->lastFrameUpdate = 0;
			this->updateAllInputsFirst = true;
			this->whenDrawOnWorldStage = WhenActive::Always;
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
		weak_ptr<Base> Base::getWeakPtr() {
			return this->shared_from_this();
		}

		//----------
		string Base::getTypeName() const {
			return "Node";
		}

		//----------
		void Base::init() {
			this->onPopulateInspector.addListener([this](ofxCvGui::InspectArguments & args) {
				this->populateInspector(args);
			}, this, 99999); // populate the inspector with this at the top. We call notify in reverse for inheritance
			this->onSerialize.addListener([this](nlohmann::json & json) {
				json["whenDrawOnWorldStage"] = (int) this->whenDrawOnWorldStage;
			}, this);
			this->onDeserialize.addListener([this](const nlohmann::json & json) {
				int whenDrawOnWorldStageInt;
				if (Utils::deserialize(json, "whenDrawOnWorldStage", whenDrawOnWorldStageInt)) {
					this->whenDrawOnWorldStage = (WhenActive::Options) whenDrawOnWorldStageInt;
				}
			}, this);
			
			//notify the subclasses to init
			this->onInit.notifyListeners();
			this->initialized = true;

			// Set the default name to our type name (but without the namespace)
			{
				auto namePath = ofSplitString(this->getTypeName(), "::");
				this->setName(namePath.back());
			}
		}

		//----------
		void Base::update() {
			auto currentFrameIndex = ofGetFrameNum() + 1; // otherwise confusions at 0th frame
			if (currentFrameIndex > this->lastFrameUpdate) {
				this->lastFrameUpdate = currentFrameIndex;
				if (this->updateAllInputsFirst) {
					for (auto inputPin : this->inputPins) {
						auto inputNode = inputPin->getConnectionUntyped();
						if (inputNode) {
							inputNode->update();
						}
					}
				}
				this->onUpdate.notifyListeners();
			}

			//check for loopback connections
			for (auto pin : this->inputPins) {
				if (pin->isConnected() && !pin->getLoopbackEnabled()) {
					//check if it's a loopback
					const auto connection = pin->getConnectionUntyped();
					if (connection.get() == this) {
						//it's a loopback, reset it
						pin->resetConnection();
					}
				}
			}
		}

		//----------
		string Base::getName() const {
			return this->name;
		}

		//----------
		void Base::setName(const string & name) {
			this->name = name;
		}

		//----------
		void Base::setNodeHost(Graph::Editor::NodeHost * nodeHost) {
			this->nodeHost = nodeHost;
		}

		//----------
		Graph::Editor::NodeHost * Base::getNodeHost() const {
			return this->nodeHost;
		}

		//----------
		const ofColor & Base::getColor() {
			if (!this->color) {
				this->color = make_shared<ofColor>(GraphicsManager::X().getColor(this->getTypeName()));
			}
			return * this->color;
		}

		//----------
		void Base::setColor(const ofColor & color) {
			this->color = make_shared<ofColor>(color);
		}

		//----------
		shared_ptr<Utils::LambdaDrawable> Base::getIcon() {
			if (!this->icon) {
				this->setIconDefault();
			}
			return this->icon;
		}

		//----------
		void Base::setIcon(shared_ptr<ofBaseDraws> icon) {
			this->icon = make_shared<Utils::LambdaDrawable>([icon](const ofRectangle& bounds) {
				icon->draw(bounds);
				});
		}

		//----------
		void Base::setIcon(shared_ptr<ofxAssets::Image> icon) {
			this->icon = make_shared<Utils::LambdaDrawable>([icon](const ofRectangle& bounds) {
				icon->get().draw(bounds);
				});
		}

		//----------
		void Base::setIconGlyph(const std::string& glyph) {
			this->icon = make_shared<Utils::LambdaDrawable>([glyph](const ofRectangle& bounds) {
				ofxCvGui::Utils::drawGlyph(glyph, bounds);
				});
		}

		//----------
		void Base::setIconDefault() {
			auto image = GraphicsManager::X().getIcon(this->getTypeName());
			this->setIcon(image);
		}

		//----------
		const Graph::PinSet & Base::getInputPins() const {
			return this->inputPins;
		}

		//----------
		void Base::populateInspector(ofxCvGui::InspectArguments &inspectArguments) {
			auto inspector = inspectArguments.inspector;
			
			auto nameWidget = inspector->add(new Widgets::Title(this->getName(), ofxCvGui::Widgets::Title::Level::H1));
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

			inspector->addToggle("Lock selection"
				, []() {
				return ofxCvGui::InspectController::X().getInspectorLocked();
			}, [](bool inspectorLocked) {
				ofxCvGui::InspectController::X().setInspectorLocked(inspectorLocked);
			});

			inspector->add(new Widgets::Title("Type : " + this->getTypeName(), ofxCvGui::Widgets::Title::Level::H3));
			
			{
				auto widget = inspector->addMultipleChoice("Draw on World Stage", { "Always", "Selected", "Never" });
				widget->setSelection(this->whenDrawOnWorldStage);
				widget->onValueChange += [this](int value) {
					this->whenDrawOnWorldStage= (WhenActive::Options) value;
				};
			}

			//pin status
			for (auto inputPin : this->getInputPins()) {
				inspector->add(new Widgets::Indicator(inputPin->getName(), [inputPin]() {
					return (Widgets::Indicator::Status) inputPin->isConnected();
				}));
			}

			//node parameters
			inspector->add(new Widgets::Spacer());
		}

		//----------
		void Base::drawWorldStage() {
			this->onDrawWorldStage.notifyListeners();
		}

		//----------
		void Base::drawWorldAdvanced(DrawWorldAdvancedArgs & args) {
			if (this->onDrawWorldAdvanced.empty()) {
				if (args.useStandardDrawWhereCustomIsNotAvailable) {
					this->drawWorldStage();
				}
			}
			else {
				this->onDrawWorldAdvanced.notifyListeners(args);
			}
		}

		//----------
		WhenActive::Options Base::getWhenDrawOnWorldStage() const {
			return this->whenDrawOnWorldStage;
		}

		//----------
		void Base::remoteControl(RemoteControllerArgs& args) {
			this->onRemoteControl.notifyListeners(args);
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
		void Base::manageParameters(ofParameterGroup & parameters, bool addToInspector) {
			this->onSerialize += [&parameters](nlohmann::json & json) {
				Utils::serialize(json, parameters);
			};
			this->onDeserialize += [&parameters](const nlohmann::json & json) {
				Utils::deserialize(json, parameters);
			};
			if (addToInspector) {
				this->onPopulateInspector += [&parameters](ofxCvGui::InspectArguments & args) {
					args.inspector->addParameterGroup(parameters);
				};
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
		void Base::setUpdateAllInputsFirst(bool updateAllInputsFirst) {
			this->updateAllInputsFirst = updateAllInputsFirst;
		}

		//----------
		bool Base::getUpdateAllInputsFirst() const {
			return this->updateAllInputsFirst;
		}
	}
}
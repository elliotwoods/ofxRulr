#pragma once

#include "../Graph/Pin.h"
#include "../Utils/Constants.h"
#include "../Utils/Serializable.h"
#include "../Exception.h"
#include "../Version.h"

#include "ofxCvGui/InspectController.h"
#include "ofxCvGui/Utils/Sugar.h"

#include "ofImage.h"
#include "ofxAssets.h"

#include <string>

#define RULR_NODE_INIT_LISTENER \
	this->onInit += [this]() { \
		this->init(); \
	}
#define RULR_NODE_UPDATE_LISTENER \
	this->onUpdate += [this]() { \
		this->update(); \
	}
#define RULR_NODE_DRAW_WORLD_LISTENER \
	this->onDrawWorld += [this]() { \
		this->drawWorld(); \
	}
#define RULR_NODE_INSPECTOR_LISTENER \
	this->onPopulateInspector += [this](ofxCvGui::InspectArguments & args) { \
		this->populateInspector(args); \
	}
#define RULR_NODE_SERIALIZATION_LISTENERS \
	this->onSerialize += [this](Json::Value & json) { \
		this->serialize(json); \
	}; \
	this->onDeserialize += [this](Json::Value const & json) { \
		this->deserialize(json); \
	}

namespace ofxRulr {
	namespace Graph {
		namespace Editor {
			class NodeHost;
		}
	}

	namespace Nodes {
		class Base : public ofxCvGui::IInspectable, public Utils::Serializable {
		public:
			Base();
			~Base();
			virtual string getTypeName() const override;
			void init();

			///Note : manually calling update more than once per frame will have no effect
			void update();

			string getName() const override;
			void setName(const string);

			void setNodeHost(Graph::Editor::NodeHost *);
			Graph::Editor::NodeHost * getNodeHost() const;

			///Calling getIcon caches the icon and the color
			shared_ptr<ofImage> getIcon();
			const ofColor & getColor();
			void setIcon(shared_ptr<ofImage>);
			void setColor(const ofColor &);

			const Graph::PinSet & getInputPins() const;
			void populateInspector(ofxCvGui::InspectArguments &);
			virtual ofxCvGui::PanelPtr getPanel() { return ofxCvGui::PanelPtr(); };

			void drawWorld();
			void drawStencil();

			template<typename NodeType>
			void connect(shared_ptr<NodeType> node) {
				auto inputPin = this->getInputPins().get<typename Graph::Pin<NodeType>>();
				if (inputPin) {
					inputPin->connect(node);
				}
				else {
					RULR_ERROR << "Couldn't connect node of type '" << NodeType().getTypeName() << "' to node '" << this->getTypeName() << "'. No matching pin found.";
				}
			}

			template<typename NodeType>
			shared_ptr<NodeType> getInput() const {
				auto pin = this->getInputPin<NodeType>();
				if (pin) {
					return pin->getConnection();
				}
				else {
					return shared_ptr<NodeType>();
				}
			}

			template<typename NodeType>
			shared_ptr<NodeType> getInput(const string & name) const {
				auto pin = this->getInputPin<NodeType>(name);
				if (pin) {
					return pin->getConnection();
				}
				else {
					return shared_ptr<NodeType>();
				}
			}

			template<typename NodeType>
			shared_ptr<Graph::Pin<NodeType>> getInputPin() const {
				return this->getInputPins().get<Graph::Pin<NodeType>>();
			}

			template<typename NodeType>
			shared_ptr<Graph::Pin<NodeType>> getInputPin(const string & name) const {
				const auto & inputPins = this->getInputPins();
				for (auto inputPin : inputPins) {
					auto typedPin = dynamic_pointer_cast<Graph::Pin<NodeType>>(inputPin);
					if (typedPin && typedPin->getName() == name) {
						return typedPin; // found the right one
					}
				}
				return shared_ptr<Graph::Pin<NodeType>>(); // didn't find
			}

			template<typename NodeType>
			void throwIfMissingAConnection() const {
				if (!this->getInput<NodeType>()) {
					stringstream message;
					message << "Node [" << this->getTypeName() << "] is missing a connection to [" << NodeType().getTypeName() << "]";
					throw(ofxRulr::Exception(message.str()));
				}
			}
			void throwIfMissingAnyConnection() const;

			void manageParameters(ofParameterGroup &, bool addToInspector = true);

			ofxLiquidEvent<void> onInit;
			ofxLiquidEvent<void> onDestroy;
			ofxLiquidEvent<void> onUpdate;
			ofxLiquidEvent<void> onDrawWorld;
			ofxLiquidEvent<void> onDrawStencil;


			ofxLiquidEvent<shared_ptr<Graph::AbstractPin>> onConnect;
			ofxLiquidEvent<shared_ptr<Graph::AbstractPin>> onDisconnect;
			ofxLiquidEvent<void> onAnyInputConnectionChanged;

		protected:
			void addInput(shared_ptr<Graph::AbstractPin>);

			template<typename NodeType>
			shared_ptr<Graph::Pin<NodeType>> addInput() {
				auto inputPin = make_shared<Graph::Pin<NodeType>>();
				this->addInput(inputPin);
				return inputPin;
			}

			template<typename NodeType>
			shared_ptr<Graph::Pin<NodeType>> addInput(const string & pinName) {
				auto inputPin = make_shared<Graph::Pin<NodeType>>(pinName);
				this->addInput(inputPin);
				return inputPin;
			}

			void removeInput(shared_ptr<Graph::AbstractPin>);
			void clearInputs();

			void setUpdateAllInputsFirst(bool);
			bool getUpdateAllInputsFirst() const;
		private:
			Graph::Editor::NodeHost * nodeHost;
			Graph::PinSet inputPins;
			shared_ptr<ofImage> icon;
			shared_ptr<ofColor> color;

			string name;
			bool initialized;
			uint64_t lastFrameUpdate;
			bool updateAllInputsFirst;
		};
	}
}
#pragma once

#include "ofxRulr/Graph/Pin.h"
#include "ofxRulr/Utils/Constants.h"
#include "ofxRulr/Utils/Serializable.h"
#include "ofxRulr/Exception.h"
#include "ofxRulr/Version.h"

#include "ofxCvGui/InspectController.h"
#include "ofxCvGui.h"

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
	this->onDrawWorldStage += [this]() { \
		this->drawWorldStage(); \
	}
#define RULR_NODE_DRAW_WORLD_ADVANCED_LISTENER \
	this->onDrawWorldAdvanced += [this](ofxRulr::DrawWorldAdvancedArgs & args) { \
		this->drawWorldAdvanced(args); \
	}
#define RULR_NODE_INSPECTOR_LISTENER \
	this->onPopulateInspector += [this](ofxCvGui::InspectArguments & args) { \
		this->populateInspector(args); \
	}
#define RULR_NODE_SERIALIZATION_LISTENERS RULR_SERIALIZE_LISTENERS

namespace ofxRulr {
	struct DrawWorldAdvancedArgs {
		bool useStandardDrawWhereCustomIsNotAvailable = true;
		bool enableGui = false;
		bool enableStyle = false;
		bool enableMaterials = false;
		bool enableColors = false;
		bool enableTextures = false;
		bool enableNormals = true;
	};

	namespace Graph {
		namespace Editor {
			class NodeHost;
		}
	}
	namespace Nodes {
		class OFXRULR_API_ENTRY Base : public ofxCvGui::IInspectable, public Utils::Serializable, protected enable_shared_from_this<Base> {
		public:
			Base();
			~Base();
			virtual string getTypeName() const override;
			void init();

			///Note : manually calling update more than once per frame will have no effect
			void update();

			string getName() const override;
			void setName(const string &);

			void setNodeHost(Graph::Editor::NodeHost *);
			Graph::Editor::NodeHost * getNodeHost() const;

			const ofColor & getColor();
			void setColor(const ofColor &);

			///Calling getIcon caches the icon and the color
			const ofBaseDraws & getIcon();
			void setIcon(shared_ptr<ofBaseDraws>); // custom icon (e.g. live)
			void setIcon(shared_ptr<ofxAssets::Image>); // standard icon

			const Graph::PinSet & getInputPins() const;
			void populateInspector(ofxCvGui::InspectArguments &);
			virtual ofxCvGui::PanelPtr getPanel() { return ofxCvGui::PanelPtr(); };

			void drawWorldStage();
			void drawWorldAdvanced(DrawWorldAdvancedArgs &);
			WhenDrawOnWorldStage::Options getWhenDrawOnWorldStage() const;
			bool checkDrawOnWorldStage(const WhenDrawOnWorldStage &);

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
			ofxLiquidEvent<void> onDrawWorldStage;
			ofxLiquidEvent<DrawWorldAdvancedArgs> onDrawWorldAdvanced;

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
			shared_ptr<ofBaseDraws> customIcon;
			shared_ptr<ofxAssets::Image> standardIcon;
			shared_ptr<ofColor> color;

			string name;
			bool initialized;
			uint64_t lastFrameUpdate;
			bool updateAllInputsFirst;

			WhenDrawOnWorldStage::Options whenDrawOnWorldStage;

			//we'd love to have parameters for drawWorldEnabled, etc
			//but adding ofParameters here seems to cause crashes
		};
	}
}
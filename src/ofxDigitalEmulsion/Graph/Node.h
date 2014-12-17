#pragma once

#include "Pin.h"
#include "../Utils/Constants.h"
#include "../Utils/Serializable.h"
#include "../Utils/Exception.h"

#include "../../../addons/ofxCvGui/src/ofxCvGui/InspectController.h"

#include "ofImage.h"
#include "ofxAssets.h"

#include <string>

#define SETUP_ICON(X) void X::setupIcon() { \
	this->setIcon(ofxAssets::image("Nodes::" + this->getTypeName())) \
}

namespace ofxDigitalEmulsion {
	namespace Graph {
		class Node : public ofxCvGui::IInspectable, public Utils::Serializable {
		public:
			Node();
			virtual void init() { };
			string getName() const override;
			void setName(const string);

			ofImage & getIcon();

			const PinSet & getInputPins() const;
			void populateInspector(ofxCvGui::ElementGroupPtr) override;
			virtual ofxCvGui::PanelPtr getView() = 0;
			virtual void update() { }

			///override this function for any node which can draw to the world
			virtual void drawWorld() { }
			///override this function to specifically define how the stencil of this layer will be drawn
			virtual void drawStencil() {
				this->drawWorld();
			}

			template<typename NodeType>
			void connect(shared_ptr<NodeType> node) {
				auto inputPin = this->getInputPins().get<Pin<NodeType>>();
				if (inputPin) {
					inputPin->connect(node);
					int dummy = 0;
					this->onConnect(dummy);
				}
				else {
					OFXDIGITALEMULSION_ERROR << "Couldn't connect node of type '" << NodeType().getTypeName() << "' to node '" << this->getTypeName() << "'. No matching pin found.";
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
			shared_ptr<Pin<NodeType>> getInputPin() const {
				return this->getInputPins().get<Pin<NodeType>>();
			}

			template<typename NodeType>
			void throwIfMissingAConnection() const {
				if (!this->getInput<NodeType>()) {
					stringstream message;
					message << "Node [" << this->getTypeName() << "] is missing a connection to [" << NodeType().getTypeName() << "]";
					throw(Utils::Exception(message.str()));
				}
			}
			void throwIfMissingAnyConnection() const;

			ofxLiquidEvent<int> onConnect;
		protected:
			void addInput(shared_ptr<AbstractPin>);

			template<typename NodeType>
			shared_ptr<Pin<NodeType>> addInput() {
				auto inputPin = make_shared<Pin<NodeType>>();
				this->addInput(inputPin);
				return inputPin;
			}

			template<typename NodeType>
			shared_ptr<Pin<NodeType>> addInput(const string & pinName) {
				auto inputPin = make_shared<Pin<NodeType>>();
				inputPin->setName(pinName);
				this->addInput(inputPin);
				return inputPin;
			}

			void removeInput(shared_ptr<AbstractPin>);
			void clearInputs();

			void setupIcon();

			virtual void populateInspector2(ofxCvGui::ElementGroupPtr) = 0;
		private:
			PinSet inputPins;
			ofImage * icon;
			string name;
			string defaultIconName;
		};
	}
}
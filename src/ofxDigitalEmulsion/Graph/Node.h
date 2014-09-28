#pragma once

#include "Pin.h"
#include "../Utils/Constants.h"
#include "../Utils/Serializable.h"
#include "../Utils/Exception.h"

#include "../../../addons/ofxCvGui2/src/ofxCvGui/Widgets/IInspectable.h"
#include <string>

namespace ofxDigitalEmulsion {
	namespace Graph {
		class Node : public ofxCvGui::Widgets::IInspectable, public Utils::Serializable {
		public:
			virtual void init() { };

			string getName() const override;
			void setName(const string);

			virtual PinSet getInputPins() const;
			void populateInspector(ofxCvGui::ElementGroupPtr) override;
			virtual ofxCvGui::PanelPtr getView() = 0;
			virtual void update() { }

			template<typename NodeType>
			void connect(shared_ptr<NodeType> node) {
				auto inputPin = this->getInputPins().get<Pin<NodeType>>();
				if (inputPin) {
					inputPin->connect(node);
					int dummy = 0;
					this->onConnect(dummy);
				} else {
					OFXDIGITALEMULSION_ERROR << "Couldn't connect node of type '" << NodeType().getTypeName() << "' to node '" << this->getTypeName() << "'. No matching pin found.";
				}
			}

			template<typename NodeType>
			shared_ptr<NodeType> getInput() const {
				auto pin = this->getInputPins().get<Pin<NodeType>>();
				if (pin) {
					return pin->getConnection();
				} else {
					return shared_ptr<NodeType>();
				}
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
			virtual void populateInspector2(ofxCvGui::ElementGroupPtr) = 0;
			string name;
		};
	}
}
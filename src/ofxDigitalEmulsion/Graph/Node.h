#pragma once

#include "Pin.h"
#include "../Utils/Constants.h"

#include "../../../addons/ofxCvGui2/src/ofxCvGui/Widgets/IInspectable.h"

namespace ofxDigitalEmulsion {
	namespace Graph {
		class Node : public ofxCvGui::Widgets::IInspectable {
		public:
			virtual string getTypeName() const = 0;
			virtual PinSet getInputPins() { return PinSet(); }
			virtual void populate(ofxCvGui::ElementGroupPtr) override { }
			virtual void update() { }
			virtual ofxCvGui::PanelPtr getView() = 0;

			template<typename NodeType>
			void connect(shared_ptr<NodeType> node) {
				auto inputPin = this->getInputPins().get<NodeType>();
				if (inputPin) {
					inputPin->connect(node);
				} else {
					OFXDIGITALEMULSION_ERROR << "Couldn't connect node of type '" << NodeType().getTypeName() << "' to node '" << this->getTypeName() << "'. No matching pin found.";
				}
			}
		};
	}
}
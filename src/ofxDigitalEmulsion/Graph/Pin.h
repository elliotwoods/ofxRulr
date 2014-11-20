#pragma once

#include "../Utils/Set.h"

#include "../../../addons/ofxLiquidEvent/src/ofxLiquidEvent.h"
#include "../../../addons/ofxCvGui/src/ofxCvGui/Element.h"

#include <string>
#include <memory>
#include <vector>
#include "ofxDigitalEmulsion/Graph/Editor/PinView.h"
using namespace std;

namespace ofxDigitalEmulsion {
	namespace Graph {
		class Node;

		class BasePin : public ofxCvGui::Element {
		public:
			BasePin(string name);
			virtual string getTypeName() = 0;
			virtual string getNodeTypeName() = 0;
			virtual void connect(shared_ptr<Node> node) = 0;
			virtual bool isConnected() const = 0;
			virtual bool checkSupports(shared_ptr<Node>) const = 0;
			virtual shared_ptr<Node> getConnectionUntyped() const = 0;

			string getName() const;
			ofVec2f getPinHeadPosition() const;

			ofxLiquidEvent<ofEventArgs> onBeginMakeConnection;
			ofxLiquidEvent<ofEventArgs> onReleaseMakeConnection;
		protected:
			shared_ptr<Editor::PinView> pinView;
		private:
			const string name;
			ofVec2f pinHeadPosition;
			ofVec2f globalElementPosition;
		};

		template<typename NodeType>
		class Pin : public BasePin {
		public:
			Pin(string name) : BasePin(name) { 
				this->pinView->setTypeName(this->getNodeTypeName());
			}
			Pin() : BasePin(this->getNodeTypeName()) { 
				this->pinView->setTypeName(this->getNodeTypeName());
			}

			string getTypeName() override { return string("Pin::") + this->getNodeTypeName(); }
			string getNodeTypeName() override { return NodeType().getTypeName(); }
			void connect(shared_ptr<NodeType> node) {
				this->connection = node;
				this->onNewConnection(node);
			}
			void connect(shared_ptr<Node> node) {
				auto castNode = dynamic_pointer_cast<NodeType>(node);
				if (!castNode) {
					throw(ofxDigitalEmulsion::Utils::Exception("Cannot connect Pin of type [" + this->getNodeTypeName() + "] to Node of type [" + node->getTypeName() + "]"));
				}
				this->connect(castNode);
			}
			shared_ptr<NodeType> getConnection() {
				return this->connection;
			}
			bool isConnected() const override {
				return (bool) this->connection;
			}
			bool checkSupports(shared_ptr<Node> node) const override {
				return (bool)dynamic_pointer_cast<NodeType>(node);
			}
			shared_ptr<Node> getConnectionUntyped() const override {
				return this->connection;
			}
			
			ofxLiquidEvent<shared_ptr<NodeType> > onNewConnection;
		protected:
			shared_ptr<NodeType> connection;
		};

		typedef Utils::Set<BasePin> PinSet;
	}
}

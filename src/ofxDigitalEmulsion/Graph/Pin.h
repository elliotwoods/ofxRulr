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

		class AbstractPin : public ofxCvGui::Element {
		public:
			AbstractPin(string name);
			virtual string getTypeName() = 0;
			virtual string getNodeTypeName() = 0;
			virtual void connect(shared_ptr<Node> node) = 0;
			virtual void resetConnection() = 0;
			virtual bool isConnected() const = 0;
			virtual bool checkSupports(shared_ptr<Node>) const = 0;
			virtual shared_ptr<Node> getConnectionUntyped() const = 0;

			virtual const ofImage & getNodeIcon() const = 0;
			virtual const ofColor & getNodeColor() const = 0;
			
			string getName() const;
			ofVec2f getPinHeadPosition() const;
			
			ofxLiquidEvent<ofEventArgs> onBeginMakeConnection;
			ofxLiquidEvent<ofxCvGui::MouseArguments> onReleaseMakeConnection;

			ofxLiquidEvent<shared_ptr<Node>> onNewConnection;
			ofxLiquidEvent<shared_ptr<Node>> onDeleteConnection;
		protected:
			shared_ptr<Editor::PinView> pinView;
		private:
			const string name;
			ofVec2f pinHeadPosition;
			ofVec2f globalElementPosition;
		};

		template<typename NodeType>
		class Pin : public AbstractPin {
		public:
			Pin(string name) : AbstractPin(name) {
				this->pinView->setup<NodeType>();
			}

			Pin() : AbstractPin(this->getNodeTypeName()) {
				this->pinView->setup<NodeType>();
			}

			~Pin() {
				this->resetConnection();
			}

			string getTypeName() override {
				return string("Pin::") + this->getNodeTypeName();
			}
			
			string getNodeTypeName() override {
				return NodeType().getTypeName();
			}

			void connect(shared_ptr<NodeType> node) {
				this->connection = node;
				this->onNewConnectionTyped(node);
				auto untypedNode = shared_ptr<Node>(node);
				this->onNewConnection(untypedNode);
			}

			void connect(shared_ptr<Node> node) override {
				auto castNode = dynamic_pointer_cast<NodeType>(node);
				if (!castNode) {
					throw(ofxDigitalEmulsion::Utils::Exception("Cannot connect Pin of type [" + this->getNodeTypeName() + "] to Node of type [" + node->getTypeName() + "]"));
				}
				this->connect(castNode);
			}

			void resetConnection() override {
				auto node = this->getConnection();
				if (node) {
					this->onDeleteConnectionTyped.notifyListeners(node);
					auto untypedNode = shared_ptr<Node>(node);
					this->onDeleteConnection.notifyListeners(untypedNode);
				}
				
				this->connection.reset();
			}
			
			shared_ptr<NodeType> getConnection() {
				return this->connection.lock();
			}
			
			bool isConnected() const override {
				return !this->connection.expired();
			}
			
			bool checkSupports(shared_ptr<Node> node) const override {
				return (bool)dynamic_pointer_cast<NodeType>(node);
			}
			
			shared_ptr<Node> getConnectionUntyped() const override {
				return this->connection.lock();
			}

			const ofImage & getNodeIcon() const override {
				//this is kind of slow, so let's check later to check
				// if this is a good location to cache a reference
				return NodeType().getIcon();
			}

			const ofColor & getNodeColor() const override {
				return NodeType().getColor();
			}
			
			ofxLiquidEvent<shared_ptr<NodeType> > onNewConnectionTyped;
			ofxLiquidEvent<shared_ptr<NodeType> > onDeleteConnectionTyped;
		protected:
			weak_ptr<NodeType> connection;
		};

		typedef Utils::Set<AbstractPin> PinSet;
	}
}

#pragma once

#include "../Utils/Set.h"

#include "../../../addons/ofxLiquidEvent/src/ofxLiquidEvent.h"
#include "../../../addons/ofxCvGui/src/ofxCvGui/Element.h"

#include <string>
#include <memory>
#include <vector>

#include "ofxRulr/Graph/Editor/PinView.h"
#include "ofxRulr/Exception.h"

using namespace std;

namespace ofxRulr {
	namespace Graph {
		class Node;

		class AbstractPin : public ofxCvGui::Element {
		public:
			AbstractPin(string name);
			virtual string getTypeName() = 0;
			virtual string getNodeTypeName() = 0;
			virtual void connect(shared_ptr<Nodes::Base> node) = 0;
			virtual void resetConnection() = 0;
			virtual bool isConnected() const = 0;
			virtual bool checkSupports(shared_ptr<Nodes::Base>) const = 0;
			virtual shared_ptr<Nodes::Base> getConnectionUntyped() const = 0;

			virtual const ofImage & getNodeIcon() const = 0;
			virtual const ofColor & getNodeColor() const = 0;
			
			string getName() const;
			ofVec2f getPinHeadPosition() const;
			
			ofxLiquidEvent<ofEventArgs> onBeginMakeConnection;
			ofxLiquidEvent<ofxCvGui::MouseArguments> onReleaseMakeConnection;

			ofxLiquidEvent<shared_ptr<Nodes::Base>> onNewConnectionUntyped;
			ofxLiquidEvent<shared_ptr<Nodes::Base>> onDeleteConnectionUntyped;
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
				this->pinView->template setup<NodeType>();
			}

			Pin() : AbstractPin(this->getNodeTypeName()) {
				this->pinView->template setup<NodeType>();
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
				this->onNewConnection(node);
				auto untypedNode = shared_ptr<Nodes::Base>(node);
				this->onNewConnectionUntyped(untypedNode);
			}

			void connect(shared_ptr<Nodes::Base> node) override {
				auto castNode = dynamic_pointer_cast<NodeType>(node);
				if (!castNode) {
					throw(ofxRulr::Exception("Cannot connect Pin of type [" + this->getNodeTypeName() + "] to Node of type [" + NodeType().getTypeName() + "]"));
				}
				this->connect(castNode);
			}

			void resetConnection() override {
				auto node = this->getConnection(); // cache the connected node before removing connection
				this->connection.reset(); //we clear the state before firing the event (e.g. for rebuilding link list)

				if (node) {
					this->onDeleteConnection.notifyListeners(node);
					auto untypedNode = shared_ptr<Nodes::Base>(node);
					this->onDeleteConnectionUntyped.notifyListeners(untypedNode);
				}	
			}
			
			shared_ptr<NodeType> getConnection() {
				return this->connection.lock();
			}
			
			bool isConnected() const override {
				return !this->connection.expired();
			}
			
			bool checkSupports(shared_ptr<Nodes::Base> node) const override {
				return (bool)dynamic_pointer_cast<NodeType>(node);
			}
			
			shared_ptr<Nodes::Base> getConnectionUntyped() const override {
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
			
			ofxLiquidEvent<shared_ptr<NodeType> > onNewConnection;
			ofxLiquidEvent<shared_ptr<NodeType> > onDeleteConnection; /// remember to check if the pointer is still valid
		protected:
			weak_ptr<NodeType> connection;
		};

		typedef Utils::Set<AbstractPin> PinSet;
	}
}

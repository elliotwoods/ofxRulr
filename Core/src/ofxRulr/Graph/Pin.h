#pragma once

#include "../Utils/Set.h"

#include "ofxLiquidEvent.h"
#include "ofxCvGui/Element.h"

#include <string>
#include <memory>
#include <vector>

#include "ofxRulr/Graph/Editor/PinView.h"
#include "ofxRulr/Exception.h"

using namespace std;

namespace ofxRulr {
	namespace Graph {
		class OFXRULR_API_ENTRY AbstractPin : public ofxCvGui::Element {
		public:
			AbstractPin(string name);
			virtual string getTypeName() = 0;
			virtual string getNodeTypeName() = 0;
			virtual void connect(shared_ptr<Nodes::Base> node) = 0;
			virtual void resetConnection() = 0;
			virtual bool isConnected() const = 0;
			virtual bool checkSupports(shared_ptr<Nodes::Base>) const = 0;
			virtual shared_ptr<Nodes::Base> getConnectionUntyped() const = 0;

			virtual const ofBaseDraws & getNodeIcon() = 0;
			virtual const ofColor & getNodeColor() const = 0;
			
			string getName() const;
			ofVec2f getPinHeadPosition() const;

			void setLoopbackEnabled(bool);
			bool getLoopbackEnabled() const;
			
			ofxLiquidEvent<ofEventArgs> onBeginMakeConnection;
			ofxLiquidEvent<ofxCvGui::MouseArguments> onReleaseMakeConnection;

			ofxLiquidEvent<shared_ptr<Nodes::Base>> onNewConnectionUntyped;
			ofxLiquidEvent<shared_ptr<Nodes::Base>> onDeleteConnectionUntyped;
		protected:
			shared_ptr<Editor::PinView> pinView;
		private:
			const string name;
			bool loopbackEnabled = false;
			ofVec2f pinHeadPosition;
			ofVec2f globalElementPosition;
		};

		template<typename NodeType>
		class Pin : public AbstractPin {
		public:
			Pin(string name) : AbstractPin(name) {
				this->pinView->template setup<NodeType>();
				auto tempNode = make_unique<NodeType>();
				this->color = tempNode->getColor();
				this->connectionMade = false;
			}

            //note we can't use 'this' in an argument to an inherited constructor
			Pin() : Pin(NodeType().getTypeName()) { }

			~Pin() {
				this->resetConnection();
			}

			string getTypeName() override {
				return string("Pin::") + this->getNodeTypeName();
			}
			
			string getNodeTypeName() override {
				return NodeType().getTypeName();
			}

			void connectTyped(shared_ptr<NodeType> node) {
				this->resetConnection();

				//trigger new connection events
				this->connection = node;
				this->onNewConnection(node);
				auto untypedNode = shared_ptr<Nodes::Base>(node);
				this->onNewConnectionUntyped(untypedNode);

				//if the node is deleted (whilst connected), then disconnect it
				//this ensures that onDeleteConnection is called
				//(note that because we use weak_ptr this isn't necessary for acceess violations)
				node->onDestroy.addListener([this]() {
					this->resetConnection();
				}, this);

				this->connectionMade = true;
			}

			void connect(shared_ptr<Nodes::Base> node) override {
				auto castNode = dynamic_pointer_cast<NodeType>(node);
				if (!castNode) {
					throw(ofxRulr::Exception("Cannot connect Pin of type [" + this->getNodeTypeName() + "] to Node of type [" + NodeType().getTypeName() + "]"));
				}
				this->connectTyped(castNode);
			}

			void resetConnection() override {
				auto node = this->getConnection(); // cache the connected node before removing connection
				this->connection.reset(); //we clear the state before firing the event (e.g. for rebuilding link list)

				if (this->connectionMade) {
					if (node) {
						this->onDeleteConnection.notifyListeners(node);
						auto untypedNode = shared_ptr<Nodes::Base>(node);
						this->onDeleteConnectionUntyped.notifyListeners(untypedNode);

						node->onDestroy.removeListeners(this);
					}
					else {
						//even if the node no longer exists, we still need to fire the event
                        auto emptyNode = shared_ptr<NodeType>();
                        auto emptyNodeUntyped = shared_ptr<Nodes::Base>();
						this->onDeleteConnection.notifyListeners(emptyNode);
						this->onDeleteConnectionUntyped.notifyListeners(emptyNodeUntyped);
					}
					this->connectionMade = false;
				}
			}
			
			shared_ptr<NodeType> getConnection() {
				return this->connection.lock();
			}
			
			/// Will return false if connected node is being destroyed even if the connection has not been fully destroyed
			bool isConnected() const override {
				return !this->connection.expired();
			}
			
			bool checkSupports(shared_ptr<Nodes::Base> node) const override {
				return (bool)dynamic_pointer_cast<NodeType>(node);
			}
			
			shared_ptr<Nodes::Base> getConnectionUntyped() const override {
				return this->connection.lock();
			}

			const ofBaseDraws & getNodeIcon() override {
				if (!this->icon) {
					this->icon = & NodeType().getIcon();
				}
				return * this->icon;
			}

			const ofColor & getNodeColor() const override {
				return this->color;
			}
			
			ofxLiquidEvent<shared_ptr<NodeType> > onNewConnection;
			ofxLiquidEvent<shared_ptr<NodeType> > onDeleteConnection; /// remember to check if the pointer is still valid
		protected:
			weak_ptr<NodeType> connection;
			bool connectionMade; // this will stay true temporarily to handle the case where connected node is deleted and weak_ptr becomes invalid
			const ofBaseDraws * icon = nullptr;
			ofColor color;
		};

		typedef Utils::Set<AbstractPin> PinSet;
	}
}

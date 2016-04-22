#include "pch_MultiTrack.h"
#include "World.h"

#include "ofxRulr/Nodes/Data/Channels/Database.h"

using namespace ofxCvGui;

namespace ofxRulr {
	namespace Nodes {
		namespace MultiTrack {
			//----------
			World::World() {
				RULR_NODE_INIT_LISTENER;
			}

			//----------
			string World::getTypeName() const {
				return "MultiTrack::World";
			}

			//----------
			void World::init() {
				//RULR_NODE_DRAW_WORLD_LISTENER;
				RULR_NODE_UPDATE_LISTENER;
				//RULR_NODE_INSPECTOR_LISTENER;
				//RULR_NODE_SERIALIZATION_LISTENERS;

				this->addInput<ofxRulr::Nodes::Data::Channels::Database>();

				for (size_t i = 0; i < NumSubscribers; i++) {
					auto subscriberPin = this->addInput<Subscriber>("Subscriber " + ofToString(i + 1));
					subscriberPin->onNewConnection += [this, i](shared_ptr<Subscriber> subscriber) {
						this->subscribers[i] = subscriber;
					};
					subscriberPin->onDeleteConnection += [this, i](shared_ptr<Subscriber> subscriber) {
						this->subscribers.erase(i);
					};
				}
			}

			//----------
			void World::update() {
				for (size_t i = 0; i < NumSubscribers; i++) {
					auto name = "Subscriber " + ofToString(i + 1);
					auto input = this->getInput<Subscriber>(name);
					if (input) {
						auto subscriber = input->getSubscriber();
						if (subscriber && subscriber->isFrameNew()) {
							// TODO Something useful here.
						}
					}
				}
			}

			//----------
			map<size_t, weak_ptr<Subscriber>> & World::getSubscribers() {
				return this->subscribers;
			}
		}
	}
}

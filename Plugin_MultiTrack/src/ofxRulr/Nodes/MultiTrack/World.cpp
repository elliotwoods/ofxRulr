#include "pch_MultiTrack.h"
#include "World.h"

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

				for (size_t i = 0; i < NumReceivers; i++) {
					auto receiverPin = this->addInput<Receiver>("Receiver " + ofToString(i + 1));
					receiverPin->onNewConnection += [this, i](shared_ptr<Receiver> receiver) {
						receivers[i] = receiver;
					};
					receiverPin->onDeleteConnection += [this, i](shared_ptr<Receiver> receiver) {
						receivers.erase(i);
					};
				}
			}

			//----------
			void World::update() {
				for (size_t i = 0; i < NumReceivers; i++) {
					auto name = "Receiver " + ofToString(i + 1);
					auto receiver = this->getInput<Receiver>(name);
					if (receiver) {
						if (receiver->getReceiver()->isFrameNew()) {
							// TODO Something useful here.
							cout << "New frame for " << name << endl;
						}
					}
					else {
						ofLogError("World::Update") << name << " does not exist!";
					}
				}
			}
		}
	}
}

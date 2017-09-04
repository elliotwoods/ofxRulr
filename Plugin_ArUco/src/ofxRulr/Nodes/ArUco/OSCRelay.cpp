#include "pch_Plugin_ArUco.h"
#include "OSCRelay.h"

#include "TrackMarkers.h"

namespace ofxRulr {
	namespace Nodes {
		namespace ArUco {
			//----------
			OSCRelay::OSCRelay() {
				RULR_NODE_INIT_LISTENER;
				this->setIcon(Nodes::GraphicsManager::X().getIcon("ArUco::Base"));
			}

			//----------
			string OSCRelay::getTypeName() const {
				return "ArUco::OSCRelay";
			}

			//----------
			void OSCRelay::init() {
				RULR_NODE_UPDATE_LISTENER;

				this->addInput<TrackMarkers>();
				this->manageParameters(this->parameters);
			}

			//----------
			void OSCRelay::update() {
				//clear sender if parameters changed
				if (this->parameters.remotePort.get() != this->cachedParameters.remotePort.get()
					|| this->parameters.remoteAddress.get() != this->cachedParameters.remoteAddress.get()) {
					this->sender.reset();
				}

				//make sender if we don't have one
				if (!this->sender) {
					auto sender = make_unique<ofxOscSender>();
					sender->setup(this->parameters.remoteAddress, this->parameters.remotePort);
					this->sender = move(sender);
					this->cachedParameters = this->parameters;
				}

				//if we have a sender then do the thing
				if (this->sender) {
					auto trackMarkersNode = this->getInput<TrackMarkers>();
					if (trackMarkersNode) {
						ofxOscBundle bundle;

						const auto & markers = trackMarkersNode->getTrackedMarkers();
						for (const auto & marker : markers) {
							ofxOscMessage message;
							message.setAddress("/aruco/" + ofToString(marker.second->ID));
							for (int i = 0; i < 16; i++) {
								message.addFloatArg(marker.second->transform.getPtr()[i]);
							}
							bundle.addMessage(message);
						}

						this->sender->sendBundle(bundle);
					}
				}
			}
		}
	}
}

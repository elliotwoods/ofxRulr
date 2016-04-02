#include "pch_MultiTrack.h"
#include "Receiver.h"

namespace ofxRulr {
	namespace Nodes {
		namespace MultiTrack {
			//----------
			Receiver::Receiver() {
				RULR_NODE_INIT_LISTENER;
			}

			//----------
			string Receiver::getTypeName() const {
				return "MultiTrack::Receiver";
			}

			//----------
			void Receiver::init() {
				RULR_NODE_INSPECTOR_LISTENER;
				RULR_NODE_SERIALIZATION_LISTENERS;

				this->panel = ofxCvGui::Panels::Groups::makeStrip();

				this->parameters.previews.enabled.addListener(this, &Receiver::previewsChangeCallback);
			}

			//----------
			void Receiver::update() {
				if (this->receiver && !this->parameters.connection.connect) {
					//disconnect
					this->receiver.reset();
				} else if(!this->receiver && this->parameters.connection.connect) {
					//connect
					this->receiver = make_shared<ofxMultiTrack::Receiver>();
					this->receiver->init(this->parameters.connection.portNumber);

					this->rebuildPreviews();
				}
			}

			//----------
			ofxCvGui::PanelPtr Receiver::getPanel() {
				return this->panel;
			}

			//----------
			void Receiver::drawWorld() {

			}

			//----------
			void Receiver::populateInspector(ofxCvGui::InspectArguments & args) {
				auto inspector = args.inspector;

				inspector->addParameterGroup(this->parameters);
			}

			//----------
			void Receiver::serialize(Json::Value & json) {
				Utils::Serializable::serialize(this->parameters, json);
			}

			//----------
			void Receiver::deserialize(const Json::Value & json) {
				Utils::Serializable::deserialize(this->parameters, json);
			}

			//----------
			void Receiver::rebuildPreviews() {
				if (this->parameters.previews.enabled) {
					this->panel->add(ofxCvGui::Panels::makePixels(this->receiver->getFrame().getColor()));
				}
				else {
					this->panel->clear();
				}
			}

			//----------
			void Receiver::previewsChangeCallback(ofParameter<bool> &) {
				this->rebuildPreviews();
			}
		}
	}
}
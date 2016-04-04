#include "pch_MultiTrack.h"
#include "Receiver.h"

using namespace ofxCvGui;

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
				RULR_NODE_DRAW_WORLD_LISTENER;
				RULR_NODE_UPDATE_LISTENER;
				RULR_NODE_INSPECTOR_LISTENER;
				RULR_NODE_SERIALIZATION_LISTENERS;

				this->panel = ofxCvGui::Panels::Groups::makeStrip();

				//this->parameters.previews.enabled.addListener(this, &Receiver::previewsChangeCallback);
			}

			//----------
			void Receiver::update() {
				bool changeConnection = false;
				if (this->receiver && !this->parameters.connection.connect) {
					//disconnect
					this->receiver.reset();
					changeConnection = true;
				} else if(!this->receiver && this->parameters.connection.connect) {
					//connect
					this->receiver = make_shared<ofxMultiTrack::Receiver>();
					this->receiver->init(this->parameters.connection.portNumber);

					changeConnection = true;
				}
				if (changeConnection) {
					this->rebuildGui();
				}

				if (this->receiver) {
					this->receiver->update();
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

				if (this->receiver) {
					inspector->addTitle("Status", Widgets::Title::Level::H2);
					inspector->addLiveValueHistory("Framerate", [this]() {
						if (this->receiver) {
							return this->receiver->getReceiver().getIncomingFramerate();
						}
						else {
							return 0.0f;
						}
					});
					inspector->addIndicator("New frame arrived", [this]() {
						if (this->receiver) {
							if (this->receiver->getReceiver().isFrameNew()) {
								return Widgets::Indicator::Status::Good;
							}
						}
						return Widgets::Indicator::Status::Clear;
					});
					inspector->addLiveValueHistory("Dropped frames", [this]() {
						if (this->receiver) {
							return (float)this->receiver->getReceiver().getDroppedFrames().size();
						}
						else {
							return 0.0f;
						}
					});
					inspector->addIndicator("New frame arrived", [this]() {
						if (this->receiver) {
							if (this->receiver->getReceiver().isFrameNew()) {
								return Widgets::Indicator::Status::Good;
							}
						}
						return Widgets::Indicator::Status::Clear;
					});
				}
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
			shared_ptr<ofxMultiTrack::Receiver> Receiver::getReceiver() const {
				return this->receiver;
			}

			//----------
			void Receiver::rebuildGui() {
				ofxCvGui::InspectController::X().refresh(this);

				this->panel->clear();
				if (this->receiver) {
					if (this->parameters.previews.enabled) {
						this->panel->add(ofxCvGui::Panels::makePixels(this->receiver->getFrame().getColor()));
					}
					else {
						this->panel->clear();
					}
				}
			}

			//----------
			void Receiver::previewsChangeCallback(ofParameter<bool> &) {
				this->rebuildGui();
			}
		}
	}
}
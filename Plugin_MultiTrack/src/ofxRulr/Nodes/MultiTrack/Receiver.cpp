#include "pch_MultiTrack.h"
#include "Receiver.h"

#include "Poco/Base64Decoder.h"

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

				this->controlSocket = make_unique<Utils::ControlSocket>();
				this->controlSocket->init("127.0.0.1", ofxMultiTrack::NodeControl);

				this->controlSocket->addHandler("getDepthToWorldTable::response", [this](const Json::Value & arguments) {
					if (!arguments["error"].empty()) {
						try {
							throw(ofxRulr::Exception(arguments["error"].toStyledString()));
						}
						RULR_CATCH_ALL_TO_ERROR;
					}
					else if(!arguments["image"].empty()) {
						try {
							string decodedMessage;

							{
								const auto encodedString = arguments["image"].asString();
								istringstream istr(encodedString);
								ostringstream ostr;
								Poco::Base64Decoder b64in(istr);
								copy(std::istreambuf_iterator<char>(b64in),
									std::istreambuf_iterator<char>(),
									std::ostreambuf_iterator<char>(ostr));

								decodedMessage = ostr.str();;
							}

							if (decodedMessage.empty()) {
								throw(ofxRulr::Exception("Can't decode Base64 message"));
							}

							ofxSquashBuddies::Message message;
							message.pushData(decodedMessage.data(), decodedMessage.size());

							if (message.empty()) {
								throw(ofxRulr::Exception("Initialise ofxSquashBuddies::Message failed"));
							}
							if (!message.getData(this->depthToCameraRays)) {
								throw(ofxRulr::Exception("Decode ofxSquashBuddies::Message to image failed"));
							}
						}
						RULR_CATCH_ALL_TO_ERROR;
					}
				});
			}

			//----------
			void Receiver::update() {
				if (this->controlSocket) {
					this->controlSocket->update();

					if (!this->depthToCameraRays.isAllocated()) {
						this->controlSocket->sendMessage("getDepthToWorldTable");
					}
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

				args.inspector->addTitle("Control Socket");
				{
					args.inspector->addIndicator("Active", [this]() {
						return (Widgets::Indicator::Status) this->controlSocket->isSocketActive();
					});
					args.inspector->addLiveValueHistory("Heartbeat age [ms]", [this]() {
						return chrono::duration_cast<chrono::milliseconds>(this->controlSocket->getTimeSinceLastHeartbeatReceived()).count();
					});
				}

				inspector->addParameterGroup(this->parameters);
			}

			//----------
			void Receiver::serialize(Json::Value & json) {
				Utils::Serializable::serialize(json, this->parameters);
			}

			//----------
			void Receiver::deserialize(const Json::Value & json) {
				Utils::Serializable::deserialize(json, this->parameters);
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
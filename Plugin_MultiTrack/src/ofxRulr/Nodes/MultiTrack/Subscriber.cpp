#include "pch_MultiTrack.h"
#include "Subscriber.h"

using namespace ofxCvGui;

namespace ofxRulr {
	namespace Nodes {
		namespace MultiTrack {
			//----------
			Subscriber::Subscriber() {
				RULR_NODE_INIT_LISTENER;
			}

			//----------
			string Subscriber::getTypeName() const {
				return "MultiTrack::Subscriber";
			}

			//----------
			void Subscriber::init() {
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
					else if (!arguments["image"].empty()) {
						//try {
						//	string decodedMessage;

						//	{
						//		const auto encodedString = arguments["image"].asString();
						//		istringstream istr(encodedString);
						//		ostringstream ostr;
						//		Poco::Base64Decoder b64in(istr);
						//		copy(std::istreambuf_iterator<char>(b64in),
						//			std::istreambuf_iterator<char>(),
						//			std::ostreambuf_iterator<char>(ostr));

						//		decodedMessage = ostr.str();;
						//	}

						//	if (decodedMessage.empty()) {
						//		throw(ofxRulr::Exception("Can't decode Base64 message"));
						//	}

						//	ofxSquashBuddies::Message message;
						//	message.pushData(decodedMessage.data(), decodedMessage.size());

						//	if (message.empty()) {
						//		throw(ofxRulr::Exception("Initialise ofxSquashBuddies::Message failed"));
						//	}
						//	if (!message.getData(this->depthToCameraLUT)) {
						//		throw(ofxRulr::Exception("Decode ofxSquashBuddies::Message to image failed"));
						//	}
						//}
						//RULR_CATCH_ALL_TO_ERROR;
					}
				});

				this->subscriber = make_shared<ofxMultiTrack::Subscriber>();
			}

			//----------
			void Subscriber::update() {
				if (this->controlSocket) {
					this->controlSocket->update();

					if (!this->depthToWorldLUT.isAllocated()) {
						this->controlSocket->sendMessage("getDepthToWorldTable");
					}
				}

				if (this->subscriber) {
					if (this->subscriber->getSubscriber().getAddress().compare(this->parameters.connection.publisherAddress) != 0 ||
						this->subscriber->getSubscriber().getPort() != this->parameters.connection.receivingPort) {
						// Address or Port value changed, reinitialize.
						this->subscriber->init(this->parameters.connection.publisherAddress, this->parameters.connection.receivingPort);
					}

					this->subscriber->update();
				}
			}

			//----------
			ofxCvGui::PanelPtr Subscriber::getPanel() {
				return this->panel;
			}

			//----------
			void Subscriber::drawWorld() {
				//this->setExtrinsics() // Directly from CV
				//this->setTransform()  // Use getTransform for the initial values in ofxNonLinearFit
				//ofxCv::estimateAffine3D()

				auto & frame = this->subscriber->getFrame();
				const auto & bodies = frame.getBodies();
				for (const auto & body : bodies) {
					body.drawWorld();
				}
			}

			//----------
			void Subscriber::populateInspector(ofxCvGui::InspectArguments & args) {
				auto inspector = args.inspector;

				if (this->subscriber) {
					inspector->addTitle("Status", Widgets::Title::Level::H2);
					inspector->addLiveValueHistory("Framerate", [this]() {
						if (this->subscriber) {
							return this->subscriber->getSubscriber().getIncomingFramerate();
						}
						else {
							return 0.0f;
						}
					});
					inspector->addIndicator("New frame arrived", [this]() {
						if (this->subscriber) {
							if (this->subscriber->getSubscriber().isFrameNew()) {
								return Widgets::Indicator::Status::Good;
							}
						}
						return Widgets::Indicator::Status::Clear;
					});
					inspector->addLiveValueHistory("Dropped frames", [this]() {
						if (this->subscriber) {
							return (float)this->subscriber->getSubscriber().getDroppedFrames().size();
						}
						else {
							return 0.0f;
						}
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

				args.inspector->addTitle("Depth to World LookUp");
				{
					args.inspector->addButton("Load from disk", [this]() {
						ofBuffer loadBuffer;
						{
							auto result = ofSystemLoadDialog("Select file", false, ofToDataPath(""));
							if (result.bSuccess) {
								auto filePath = result.filePath;
								loadBuffer = ofBufferFromFile(filePath);
							}
						}

						if (loadBuffer.size()) {
							ofxSquashBuddies::Message message;
							message.pushData(loadBuffer.getData(), loadBuffer.size());
							message.getData(this->depthToWorldLUT);
						}
					});
				}

				inspector->addParameterGroup(this->parameters);
				this->parameters.previews.enabled.addListener(this, &Subscriber::previewsChangeCallback);
			}

			//----------
			void Subscriber::serialize(Json::Value & json) {
				Utils::Serializable::serialize(json, this->parameters);
			}

			//----------
			void Subscriber::deserialize(const Json::Value & json) {
				Utils::Serializable::deserialize(json, this->parameters);
			}

			//----------
			shared_ptr<ofxMultiTrack::Subscriber> Subscriber::getSubscriber() const {
				return this->subscriber;
			}

			//----------
			const ofFloatPixels & Subscriber::getDepthToWorldLUT() const {
				return this->depthToWorldLUT;
			}

			//----------
			void Subscriber::rebuildGui() {
				ofxCvGui::InspectController::X().refresh(this);

				this->panel->clear();
				if (this->subscriber) {
					if (this->parameters.previews.enabled) {
						this->panel->add(ofxCvGui::Panels::makePixels(this->subscriber->getFrame().getDepth()));
					}
					else {
						this->panel->clear();
					}
				}
			}

			//----------
			void Subscriber::previewsChangeCallback(bool &) {
				this->rebuildGui();
			}
		}
	}
}
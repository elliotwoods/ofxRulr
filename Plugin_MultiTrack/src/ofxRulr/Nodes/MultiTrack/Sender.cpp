#ifdef OFXMULTITRACK_UDP

#include "pch_MultiTrack.h"
#include "Sender.h"
#include "ofxRulr/Nodes/Item/KinectV2.h"

#include "Poco/Base64Decoder.h"
#include "Poco/Base64Encoder.h"

using namespace ofxCvGui;

namespace ofxRulr {
	namespace Nodes {
		namespace MultiTrack {
			//----------
			Sender::Sender() {
				RULR_NODE_INIT_LISTENER;
			}

			//----------
			string Sender::getTypeName() const {
				return "MultiTrack::Sender";
			}

			//----------
			void Sender::init() {
				RULR_NODE_UPDATE_LISTENER;
				RULR_NODE_INSPECTOR_LISTENER;
				RULR_NODE_SERIALIZATION_LISTENERS;

				auto kinectInput = this->addInput<Item::KinectV2>();
				kinectInput->onNewConnection += [this](shared_ptr<Item::KinectV2> & kinectNode) {
					auto kinect = kinectNode->getDevice();
					if (kinect) {
						this->sender = make_shared<ofxMultiTrack::Sender>();
						this->sender->init(*kinectNode->getDevice(), this->parameters.target.ipAddress, this->parameters.target.port);
					}
					ofxCvGui::refreshInspector(this);
				};
				kinectInput->onDeleteConnection += [this](shared_ptr<Item::KinectV2> &) {
					this->sender.reset();
					ofxCvGui::refreshInspector(this);
				};

				this->buildControlSocket();
			}

			//----------
			void Sender::update() {
				if (this->controlSocket) {
					this->controlSocket->update();
				}

				if (this->sender) {
					auto kinect = this->getInput<Item::KinectV2>()->getDevice();

					//this should anyway be true if sender exists
					if (kinect) {
						//sync endpoint parameters
						auto endPoint = this->sender->getSender().getEndPoint();
						if (this->parameters.target.ipAddress.get() != endPoint.getEndPoint().address().to_string()
							|| this->parameters.target.port.get() != endPoint.getEndPoint().port()) {
							this->sender->init(*kinect, this->parameters.target.ipAddress, this->parameters.target.port);
						}

						//sync parameters
						if (this->sender->getSender().getPacketSize() != this->parameters.squashBuddies.packetSize) {
							this->sender->getSender().setPacketSize(this->parameters.squashBuddies.packetSize);
						}
						if (this->sender->getSender().getMaxSocketBufferSize() != this->parameters.squashBuddies.maxSocketBufferSize) {
							this->sender->getSender().setMaxSocketBufferSize(this->parameters.squashBuddies.maxSocketBufferSize);
						}

						this->sender->update();
					}
				}
			}

			//----------
			void Sender::populateInspector(ofxCvGui::InspectArguments & args) {
				if (this->sender) {
					args.inspector->addLiveValueHistory("Sender FPS", [this]() {
						return this->sender->getSender().getSendFramerate();
					});
				}

				if (this->controlSocket) {
					args.inspector->addTitle("Control Socket");
					{
						args.inspector->addIndicator("Active", [this]() {
							return (Widgets::Indicator::Status) this->controlSocket->isSocketActive();
						});
						args.inspector->addLiveValueHistory("Heartbeat age [ms]", [this]() {
							return chrono::duration_cast<chrono::milliseconds>(this->controlSocket->getTimeSinceLastHeartbeatReceived()).count();
						});
					}
				}

				args.inspector->addParameterGroup(this->parameters);
			}

			//----------
			void Sender::serialize(Json::Value & json) {
				Utils::Serializable::serialize(json, this->parameters);
			}

			//----------
			void Sender::deserialize(const Json::Value & json) {
				Utils::Serializable::deserialize(json, this->parameters);
			}

			//----------
			shared_ptr<ofxMultiTrack::Sender> Sender::getSender() {
				return this->sender;
			}

			//----------
			void Sender::buildControlSocket() {
				if (this->parameters.controlSocket.enabled) {
					this->controlSocket = make_unique<Utils::ControlSocket>();
					this->controlSocket->init(this->parameters.controlSocket.port);

					this->controlSocket->addHandler("setTarget", [this](const Json::Value & json) {
						Utils::Serializable::deserialize(json, this->parameters.target);
					});

					this->controlSocket->addHandler("getDepthToWorldTable", [this](const Json::Value &) {
						try {
							this->throwIfMissingAConnection<Item::KinectV2>();
							auto kinect = this->getInput<Item::KinectV2>();

							auto depthSource = kinect->getDevice()->getDepthSource();
							if (!depthSource) {
								throw(ofxRulr::Exception("No depth source available"));
							}

							ofFloatPixels depthToWorldTable;
							depthSource->getDepthToWorldTable(depthToWorldTable);

							if (!depthToWorldTable.isAllocated()) {
								throw(ofxRulr::Exception("depthToWorldTable is empty"));
							}
							ofxSquashBuddies::Message pixelsMessage(depthToWorldTable);

							auto messageString = pixelsMessage.getMessageString();

							stringstream base64StringStream;
							Poco::Base64Encoder(base64StringStream) << messageString;
							auto base64String = base64StringStream.str();

							Json::Value arguments;
							arguments["image"] = base64String;

							{
								const auto encodedString = arguments["image"].asString();
								istringstream istr(encodedString);
								ostringstream ostr;
								Poco::Base64Decoder b64in(istr);
								copy(std::istreambuf_iterator<char>(b64in),
									std::istreambuf_iterator<char>(),
									std::ostreambuf_iterator<char>(ostr));

								auto decodedMessage = ostr.str();;
							}

							this->controlSocket->sendMessage("getDepthToWorldTable::response", arguments);
						}
						catch(ofxRulr::Exception e) {
							Json::Value arguments;
							arguments["error"] = e.what();
							this->controlSocket->sendMessage("getDepthToWorldTable::response", arguments);
						}
					});
				}
				else {
					this->controlSocket.reset();
				}

				ofxCvGui::refreshInspector(this);
			}
		}
	}
}

#endif // OFXMULTITRACK_UDP
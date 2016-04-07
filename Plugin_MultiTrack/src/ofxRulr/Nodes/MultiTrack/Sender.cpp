#include "pch_MultiTrack.h"
#include "Sender.h"
#include "ofxRulr/Nodes/Item/KinectV2.h"

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

				this->controlSocket = make_unique<Utils::ControlSocket>();
				this->controlSocket->addHandler("setTarget", [this](const Json::Value & json) {
					Utils::Serializable::deserialize(this->parameters.target, json);
				});

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
			}

			//----------
			void Sender::update() {
				this->controlSocket->update();

				if (this->sender) {
					auto kinect = this->getInput<Item::KinectV2>()->getDevice();

					//this should also be true
					if (kinect) {
						//sync endpoint parameters
						auto endPoint = this->sender->getSender().getEndPoint();
						if (this->parameters.target.ipAddress.get() != endPoint.getEndPoint().address().to_string()
							|| this->parameters.target.port.get() != endPoint.getEndPoint().port()) {
							this->sender->init(*kinect, this->parameters.target.ipAddress, this->parameters.target.port);
						}

						//sync parameters
						if (this->sender->getSender().getPacketSize() != this->parameters.packetSize) {
							this->sender->getSender().setPacketSize(this->parameters.packetSize);
						}
						if (this->sender->getSender().getMaxSocketBufferSize() != this->parameters.maxSocketBufferSize) {
							this->sender->getSender().setMaxSocketBufferSize(this->parameters.maxSocketBufferSize);
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

				args.inspector->addParameterGroup(this->parameters);
			}

			//----------
			void Sender::serialize(Json::Value & json) {
				Utils::Serializable::serialize(this->parameters, json);
			}

			//----------
			void Sender::deserialize(const Json::Value & json) {
				Utils::Serializable::deserialize(this->parameters, json);
			}

			//----------
			shared_ptr<ofxMultiTrack::Sender> Sender::getSender() {
				return this->sender;
			}
		}
	}
}
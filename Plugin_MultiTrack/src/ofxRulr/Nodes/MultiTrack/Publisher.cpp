#include "pch_MultiTrack.h"
#include "Publisher.h"

#include "ofxRulr/Nodes/Item/KinectV2.h"

//#include "Poco/Base64Encoder.h"

using namespace ofxCvGui;

namespace ofxRulr {
	namespace Nodes {
		namespace MultiTrack {
			//----------
			Publisher::Publisher() {
				RULR_NODE_INIT_LISTENER;
			}

			//----------
			string Publisher::getTypeName() const {
				return "MultiTrack::Publisher";
			}

			//----------
			void Publisher::init() {
				RULR_NODE_UPDATE_LISTENER;
				RULR_NODE_INSPECTOR_LISTENER;
				RULR_NODE_SERIALIZATION_LISTENERS;

				auto kinectInput = this->addInput<Item::KinectV2>();
				kinectInput->onNewConnection += [this](shared_ptr<Item::KinectV2> & kinectNode) {
					auto kinect = kinectNode->getDevice();
					if (kinect) {
						this->publisher = make_shared<ofxMultiTrack::Publisher>();
						this->publisher->init(*kinectNode->getDevice(), this->parameters.dataSocket.port);
					}
					ofxCvGui::refreshInspector(this);
				};
				kinectInput->onDeleteConnection += [this](shared_ptr<Item::KinectV2> &) {
					this->publisher.reset();
					ofxCvGui::refreshInspector(this);
				};

				this->buildControlSocket();
			}

			//----------
			void Publisher::update() {
				if (this->controlSocket) {
					this->controlSocket->update();
				}

				if (this->publisher) {
					auto kinect = this->getInput<Item::KinectV2>()->getDevice();

					//this should anyway be true if publisher exists
					if (kinect) {
						//sync endpoint parameters
						if (this->parameters.dataSocket.port.get() != this->publisher->getPublisher().getPort()) {
							this->publisher->init(*kinect, this->parameters.dataSocket.port);
						}

						////sync parameters
						//if (this->publisher->getPublisher().getPacketSize() != this->parameters.squashBuddies.packetSize) {
						//	this->publisher->getPublisher().setPacketSize(this->parameters.squashBuddies.packetSize);
						//}
						//if (this->publisher->getPublisher().getMaxSocketBufferSize() != this->parameters.squashBuddies.maxSocketBufferSize) {
						//	this->publisher->getPublisher().setMaxSocketBufferSize(this->parameters.squashBuddies.maxSocketBufferSize);
						//}

						this->publisher->update();
					}
				}
			}

			//----------
			void Publisher::populateInspector(ofxCvGui::InspectArguments & args) {
				if (this->publisher) {
					args.inspector->addLiveValueHistory("Publisher FPS", [this]() {
						return this->publisher->getPublisher().getSendFramerate();
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

				args.inspector->addLiveValueHistory("Kinect device framerate", [this]() {
					return this->publisher->getDeviceFrameRate();
				});
				args.inspector->addLiveValueHistory("Sending framerate", [this]() {
					return this->publisher->getPublisher().getSendFramerate();
				});
				args.inspector->addLiveValueHistory("Dropped frame", [this]() {
					return (float) this->droppedFrame;
				});
				args.inspector->addEditableValue<int>("Packet size", [this]() {
					return (int) this->publisher->getPublisher().getPacketSize();
				}, [this](string newSizeString) {
					if (!newSizeString.empty()) {
						this->publisher->getPublisher().setPacketSize(ofToInt(newSizeString));
					}
				});
				args.inspector->addLiveValue<size_t>("Current compressor queue size", [this]() {
					return this->publisher->getPublisher().getCurrentCompressorQueueSize();
				});
				args.inspector->addLiveValue<size_t>("Current socket buffer size", [this]() {
					return this->publisher->getPublisher().getCurrentSocketBufferSize();
				});
				args.inspector->addEditableValue<int>("Max socket buffer size", [this]() {
					return (int) this->publisher->getPublisher().getMaxSocketBufferSize();
				}, [this](string newSizeString) {
					if (!newSizeString.empty()) {
						this->publisher->getPublisher().setMaxSocketBufferSize(ofToInt(newSizeString));
					}
				});

				args.inspector->addParameterGroup(this->parameters);

				args.inspector->addButton("Save Depth To World Table", [this]() {
					string filename = "DepthToWorld.raw";
					{
						auto result = ofSystemTextBoxDialog("Enter filename [" + filename + "]");
						if (!result.empty()) {
							filename = result;
						}

						string saveString;
						{
							ofFloatPixels depthToWorldTable;
							auto kinect = this->getInput<Item::KinectV2>()->getDevice();
							kinect->getDepthSource()->getDepthToWorldTable(depthToWorldTable);
							ofxSquashBuddies::Message message;
							message.setData(depthToWorldTable);
							saveString = message.getMessageString();
							ofBuffer saveBuffer(saveString);
							if (!ofBufferToFile(ofToDataPath(filename), saveBuffer)) {
								ofSystemAlertDialog("Error saving table to disk!");
							}
						}
					}
				});
			}

			//----------
			void Publisher::serialize(Json::Value & json) {
				Utils::Serializable::serialize(json, this->parameters);
			}

			//----------
			void Publisher::deserialize(const Json::Value & json) {
				Utils::Serializable::deserialize(json, this->parameters);
			}

			//----------
			shared_ptr<ofxMultiTrack::Publisher> Publisher::getPublisher() {
				return this->publisher;
			}

			//----------
			void Publisher::buildControlSocket() {
				if (this->parameters.controlSocket.enabled) {
					this->controlSocket = make_unique<Utils::ControlSocket>();
					this->controlSocket->init(this->parameters.controlSocket.port);

					this->controlSocket->addHandler("setDataSocket", [this](const Json::Value & json) {
						Utils::Serializable::deserialize(json, this->parameters.dataSocket);
					});

					this->controlSocket->addHandler("getDepthToWorldTable", [this](const Json::Value &) {
						//try {
						//	this->throwIfMissingAConnection<Item::KinectV2>();
						//	auto kinect = this->getInput<Item::KinectV2>();

						//	auto depthSource = kinect->getDevice()->getDepthSource();
						//	if (!depthSource) {
						//		throw(ofxRulr::Exception("No depth source available"));
						//	}

						//	ofFloatPixels depthToWorldTable;
						//	depthSource->getDepthToWorldTable(depthToWorldTable);

						//	if (!depthToWorldTable.isAllocated()) {
						//		throw(ofxRulr::Exception("depthToWorldTable is empty"));
						//	}
						//	ofxSquashBuddies::Message pixelsMessage(depthToWorldTable);

						//	auto messageString = pixelsMessage.getMessageString();

						//	stringstream base64StringStream;
						//	Poco::Base64Encoder(base64StringStream) << messageString;
						//	auto base64String = base64StringStream.str();

						//	Json::Value arguments;
						//	arguments["image"] = base64String;

						//	{
						//		const auto encodedString = arguments["image"].asString();
						//		istringstream istr(encodedString);
						//		ostringstream ostr;
						//		Poco::Base64Decoder b64in(istr);
						//		copy(std::istreambuf_iterator<char>(b64in),
						//			std::istreambuf_iterator<char>(),
						//			std::ostreambuf_iterator<char>(ostr));

						//		auto decodedMessage = ostr.str();;
						//	}

						//	this->controlSocket->sendMessage("getDepthToWorldTable::response", arguments);
						//}
						//catch (ofxRulr::Exception e) {
						//	Json::Value arguments;
						//	arguments["error"] = e.what();
						//	this->controlSocket->sendMessage("getDepthToWorldTable::response", arguments);
						//}
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
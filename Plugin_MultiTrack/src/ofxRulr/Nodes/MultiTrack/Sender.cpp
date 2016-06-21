#include "pch_MultiTrack.h"

#include "Sender.h"

#include "ofxRulr/Nodes/Item/KinectV2.h"
#include "ofxRulr/Nodes/Item/Camera.h"

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

				this->needsRebuild = false;

				auto kinectInput = this->addInput<Item::KinectV2>();
				kinectInput->onNewConnection += [this](shared_ptr<Item::KinectV2> & kinectNode) {
					// TODO: How do I remove this event listener in onDeleteConnection?
					kinectNode->onSourcesChanged += [this]() {
						this->needsRebuild = true;
					};

					this->needsRebuild = true;
				};
				kinectInput->onDeleteConnection += [this](shared_ptr<Item::KinectV2> &) {
					this->sender.reset();
					ofxCvGui::refreshInspector(this);
				};

				auto cameraInput = this->addInput<Item::Camera>();
				cameraInput->onNewConnection += [this](shared_ptr<Item::Camera> &) {
					this->needsRebuild = true;
				};
				cameraInput->onDeleteConnection += [this](shared_ptr<Item::Camera> &) {
					this->needsRebuild = true;
				};

				this->buildControlSocket();
			}

			//----------
			void Sender::update() {
				if (this->needsRebuild) {
					this->rebuild();
				}

				if (this->controlSocket) {
					this->controlSocket->update();
				}

				if (this->sender) {
					auto kinect = this->getInput<Item::KinectV2>()->getDevice();

					//this should anyway be true if sender exists
					if (kinect) {
						//sync endpoint parameters
						if (this->parameters.dataSocket.ipAddress.get() != this->sender->getSender().getAddress()
							|| this->parameters.dataSocket.port.get() != this->sender->getSender().getPort()) {
							this->sender->getSender().init(this->parameters.dataSocket.ipAddress, this->parameters.dataSocket.port);
						}

						//sync parameters
						if (this->sender->getSender().getPacketSize() != this->parameters.squashBuddies.packetSize) {
							this->sender->getSender().setPacketSize(this->parameters.squashBuddies.packetSize);
						}
						if (this->sender->getSender().getMaxSocketBufferSize() != this->parameters.squashBuddies.maxSocketBufferSize) {
							this->sender->getSender().setMaxSocketBufferSize(this->parameters.squashBuddies.maxSocketBufferSize);
						}

						this->droppedFrame = !this->sender->update();
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

				if (this->sender) {
					args.inspector->addLiveValueHistory("Kinect device framerate", [this]() {
						return this->sender->getDeviceFrameRate();
					});
					args.inspector->addLiveValueHistory("Sending framerate", [this]() {
						return this->sender->getSender().getSendFramerate();
					});
					args.inspector->addLiveValueHistory("Dropped frame", [this]() {
						return (float) this->droppedFrame;
					});
					args.inspector->addLiveValue<size_t>("Current socket buffer size", [this]() {
						return this->sender->getSender().getCurrentSocketBufferSize();
					});
				}

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
			void Sender::rebuild() {
				auto kinectNode = this->getInput<Item::KinectV2>();
				auto kinect = kinectNode->getDevice();
				if (kinect) {
					auto cameraNode = this->getInput<Item::Camera>();
					if (cameraNode) {
						auto grabber = cameraNode->getGrabber();
						if (grabber) {
							//Sender with external color
							auto publisherExtColor = make_shared<ofxMultiTrack::SenderExtColor>();
							publisherExtColor->init(kinect, grabber, this->parameters.dataSocket.ipAddress, this->parameters.dataSocket.port);
							auto camera = cameraNode->getViewInWorldSpace();
							publisherExtColor->setCameraParams(cameraNode->getDistortionCoefficients(), camera.getViewMatrix(), camera.getClippedProjectionMatrix());
							this->sender = publisherExtColor;
						}
						else {
							//Sender with device color
							this->sender = make_shared<ofxMultiTrack::Sender>();
							this->sender->init(kinect, this->parameters.dataSocket.ipAddress, this->parameters.dataSocket.port);
						}
					}
					else {
						//Sender with device color
						this->sender = make_shared<ofxMultiTrack::Sender>();
						this->sender->init(kinect, this->parameters.dataSocket.ipAddress, this->parameters.dataSocket.port);
					}
				}
				else {
					this->sender.reset();
				}

				this->needsRebuild = false;

				ofxCvGui::refreshInspector(this);
			}

			//----------
			void Sender::buildControlSocket() {
				if (this->parameters.controlSocket.enabled) {
					this->controlSocket = make_unique<Utils::ControlSocket>();
					this->controlSocket->init(this->parameters.controlSocket.port);

					this->controlSocket->addHandler("setDataSocket", [this](const Json::Value & json) {
						Utils::Serializable::deserialize(json, this->parameters.dataSocket);
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

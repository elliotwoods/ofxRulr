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

				this->previewPanel = ofxCvGui::Panels::makeTexture(this->previewTexture);
				this->previewPanel->setInputRange(0.0f, 8000.0f / 0xffff);

				this->uiPanel = ofxCvGui::Panels::makeWidgets();
				this->uiPanel->addLiveValue<float>("Framerate", [this]() {
					if (this->subscriber) {
						return this->subscriber->getSubscriber().getIncomingFramerate();
					}
					else {
						return 0.0f;
					}
				});
				this->uiPanel->addIndicator("New frame arrived", [this]() {
					if (this->subscriber) {
						if (this->subscriber->getSubscriber().isFrameNew()) {
							return Widgets::Indicator::Status::Good;
						}
					}
					return Widgets::Indicator::Status::Clear;
				});
				this->uiPanel->addLiveValue<float>("Dropped frames", [this]() {
					if (this->subscriber) {
						return (float)this->subscriber->getSubscriber().getDroppedFrames().size();
					}
					else {
						return 0.0f;
					}
				});
				this->uiPanel->addListenersToParent(this->previewPanel);
				this->previewPanel->onBoundsChange += [this](ofxCvGui::BoundsChangeArguments& args) {
					auto bounds = args.localBounds;
					bounds.x = 20;
					bounds.y = 70;
					bounds.width = MIN(bounds.width - 20, 200);
					bounds.height = bounds.height - 120;
					this->uiPanel->setBounds(bounds);
				};
				this->uiPanel->onDraw.addListener([](ofxCvGui::DrawArguments & args) {
					ofPushStyle();
					{
						ofSetColor(0, 200);
						ofDrawRectangle(args.localBounds);
					}
					ofPopStyle();
				}, this, -200);

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

				static auto inc = 11;
				this->parameters.connection.publisherAddress = "10.0.0." + ofToString(inc++);

				this->parameters.calibration.depthToWorldTableFile.addListener(this, &Subscriber::depthToWorldTableFileCallback);

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

					if (this->subscriber->getSubscriber().isFrameNew()) {
						auto & pixels = this->subscriber->getFrame().getDepth();
						if (!this->previewTexture.isAllocated()) {
							this->previewTexture.allocate(pixels);
						}
						this->previewTexture.loadData(pixels);
					}
				}
			}

			//----------
			ofxCvGui::PanelPtr Subscriber::getPanel() {
				return this->previewPanel;
			}

			//----------
			void Subscriber::drawWorld() {
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

				args.inspector->addTitle("Calibration");
				{
					args.inspector->addIndicator("Depth to World loaded", [this]() {
						if (this->depthToWorldLUT.isAllocated()) {
							if (this->depthToWorldLUT.getWidth() == 512 && this->depthToWorldLUT.getHeight() == 424) {
								return ofxCvGui::Widgets::Indicator::Status::Good;
							}
							else {
								return ofxCvGui::Widgets::Indicator::Status::Warning;
							}
						}
						else {
							return ofxCvGui::Widgets::Indicator::Status::Clear;
						}
					});
					args.inspector->addButton("Select file", [this]() {
						auto result = ofSystemLoadDialog("Select Depth to World LUT");
						if (result.bSuccess) {
							this->parameters.calibration.depthToWorldTableFile = result.filePath;
						}
					});
				}

				inspector->addParameterGroup(this->parameters);
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
			const ofTexture & Subscriber::getPreviewTexture() const {
				return this->previewTexture;
			}

			//----------
			void Subscriber::depthToWorldTableFileCallback(string & filename) {
				if (!filename.empty()) {
					try {
						this->loadDepthToWorldTableFile();
					}
					RULR_CATCH_ALL_TO_ALERT;
				}
			}

			//----------
			void Subscriber::loadDepthToWorldTableFile() {
				const auto & filename = this->parameters.calibration.depthToWorldTableFile.get();
				if (!ofFile::doesFileExist(filename)) {
					throw(ofxRulr::Exception("File not found : " + filename));
				}

				auto loadBuffer = ofBufferFromFile(filename);
				if (!loadBuffer.size()) {
					throw(ofxRulr::Exception("File empty"));
				}

				ofxSquashBuddies::Message message;
				message.pushData(loadBuffer.getData(), loadBuffer.size());
				message.getData(this->depthToWorldLUT);
			}
		}
	}
}
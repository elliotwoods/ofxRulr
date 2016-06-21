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
			Subscriber::~Subscriber() {
				
			}

			//----------
			string Subscriber::getTypeName() const {
				return "MultiTrack::Subscriber";
			}

			//----------
			void Subscriber::init() {
				RULR_RIGIDBODY_DRAW_OBJECT_LISTENER;
				RULR_NODE_UPDATE_LISTENER;
				RULR_NODE_INSPECTOR_LISTENER;
				RULR_NODE_SERIALIZATION_LISTENERS;

				this->previewPanel = ofxCvGui::Panels::makeTexture(this->depthTexture);
				this->previewPanel->setInputRange(0.0f, 8000.0f / 0xffff);
				
				//apply color to the preview panel
				this->previewPanel->onDrawImage.addListener([this](ofxCvGui::DrawImageArguments &) {
					if (this->parameters.draw.gpuPointCloud.style.applyIndexColor) {
						ofPushStyle();
						ofSetColor(this->getSubscriberColor());
					}
				}, this, -1);
				this->previewPanel->onDrawImage.addListener([this](ofxCvGui::DrawImageArguments &) {
					if (this->parameters.draw.gpuPointCloud.style.applyIndexColor) {
						ofPopStyle();
					}
				}, this, +1);
				this->uiPanel = ofxCvGui::Panels::makeWidgets();
				this->uiPanel->addLiveValue<float>("Framerate", [this]() {
					if (this->subscriber) {
						return this->subscriber->getSubscriber().getIncomingFramerate();
					}
					else {
						return 0.0f;
					}
				});
				this->uiPanel->addToggle(this->parameters.draw.gpuPointCloud.style.applyIndexColor);
				this->uiPanel->addIndicator("Depth to World loaded", [this]() {
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

				static auto inc = 11;
				this->parameters.connection.publisherAddress = "10.0.0." + ofToString(inc++);

				this->parameters.calibration.depthToWorldTableFile.addListener(this, &Subscriber::depthToWorldTableFileCallback);

				static int colorCounter = 0;
				this->debugColor.setHsb(ofRandom(0.05f) + (0.15f * colorCounter++), 1.0f, 1.0f);
			}

			//----------
			void Subscriber::update() {
				//connect/disconnect
				if (!this->subscriber && this->parameters.connection.connect) {
					this->subscriber = make_shared<ofxMultiTrack::Subscriber>();
					this->subscriber->init(this->parameters.connection.publisherAddress, this->parameters.connection.publisherPort);
				}
				else if (this->subscriber && !this->parameters.connection.connect) {
					this->subscriber.reset();
				}

				//change connection properties
				if (this->subscriber) {
					if (this->subscriber->getSubscriber().getAddress().compare(this->parameters.connection.publisherAddress) != 0 ||
						this->subscriber->getSubscriber().getPort() != this->parameters.connection.publisherPort) {

						this->subscriber->init(this->parameters.connection.publisherAddress, this->parameters.connection.publisherPort);
					}
				}

				if (this->subscriber) {
					if (this->subscriber->getSubscriber().getAddress().compare(this->parameters.connection.publisherAddress) != 0 ||
						this->subscriber->getSubscriber().getPort() != this->parameters.connection.publisherPort) {
						// Address or Port value changed, reinitialize.
						this->subscriber->init(this->parameters.connection.publisherAddress, this->parameters.connection.publisherPort);
					}

					this->subscriber->update();

					if (this->subscriber->getSubscriber().isFrameNew()) {
						const auto & depthPixels = this->subscriber->getFrame().getDepth();
						const auto & irPixels = this->subscriber->getFrame().getInfrared();

						if (depthPixels.isAllocated()) {
							this->depthTexture.loadData(depthPixels);
						}
						if (irPixels.isAllocated()) {
							this->irTexture.loadData(irPixels);
						}

						//Mesh update.
						if (this->parameters.draw.gpuPointCloud.enabled) {
							auto & meshDimensions = this->meshProvider.getDimensions();
							if (meshDimensions.x != depthPixels.getWidth() || meshDimensions.y != depthPixels.getHeight()) {
								this->meshProvider.setDimensions(ofVec2f(depthPixels.getWidth(), depthPixels.getHeight()));
							}

							if (this->parameters.draw.gpuPointCloud.downsampleExp != this->meshProvider.getDownsampleExp()) {
								this->meshProvider.setDownsampleExp(this->parameters.draw.gpuPointCloud.downsampleExp);
							}
						}
					}
				}
			}

			//----------
			ofxCvGui::PanelPtr Subscriber::getPanel() {
				return this->previewPanel;
			}

			//----------
			void Subscriber::drawObject() {
				if (this->parameters.draw.bodies) {
					ofPushStyle();
					{
						ofSetColor(this->debugColor);

						if (this->subscriber) {
							const auto & bodies = this->subscriber->getFrame().getBodies();
							for (const auto & body : bodies) {
								body.drawWorld(); // actually this is in kinect camera space
							}
						}
					}
					ofPopStyle();
				}

				if (this->parameters.draw.gpuPointCloud.enabled) {
					PointCloudStyle style;
					style.applyIR = this->parameters.draw.gpuPointCloud.style.applyIR;
					style.applyIndexColor = this->parameters.draw.gpuPointCloud.style.applyIndexColor;
					this->drawPointCloudGpu(style);
				}

				if (this->parameters.draw.cpuPointCloud.enabled) {
					this->drawPointCloudCpu();
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
			const ofTexture & Subscriber::getDepthTexture() const {
				return this->depthTexture;
			}

			//----------
			const ofTexture & Subscriber::getIRTexture() const {
				return this->irTexture;
			}

			//----------
			const ofFloatColor & Subscriber::getSubscriberColor() const {
				return this->debugColor;
			}

			//----------
			void Subscriber::drawPointCloudCpu() {
				if (this->depthToWorldLUT.isAllocated() && this->subscriber) {
					const auto & frame = this->subscriber->getFrame();

					vector<ofVec3f> vertices;
					vector<ofFloatColor> colors;

					auto depth = frame.getDepth().getData();

					auto useIR = this->parameters.draw.cpuPointCloud.applyIRTexture.get() && frame.getInfrared().isAllocated();
					auto IR = frame.getInfrared().getData();
					auto depthToXY = this->depthToWorldLUT.getData();

					auto colorAmplitude = this->parameters.draw.IRAmplitude;

					for (int i = 0; i < frame.getDepth().size(); i++) {
						vertices.emplace_back(ofVec3f{
							(float)depth[i] * depthToXY[i * 2 + 0] / 1000.0f,
							(float)depth[i] * depthToXY[i * 2 + 1] / 1000.0f,
							(float)depth[i] / 1000.0f
						});
						if (useIR) {
							colors.emplace_back(ofFloatColor{
								(float)IR[i] / (float)0xffff * colorAmplitude
							});
						}
					}
					ofVbo vbo;
					vbo.setVertexData(vertices.data(), vertices.size(), GL_STATIC_DRAW);
					if (useIR) {
						vbo.setColorData(colors.data(), colors.size(), GL_STATIC_DRAW);
					}
					vbo.draw(GL_POINTS, 0, vertices.size());
				}
			}

			//----------
			void Subscriber::drawPointCloudGpu(PointCloudStyle pointCloudStyle) {
				auto & worldShader = this->getWorldShader();
				worldShader.begin();
				{
					worldShader.setUniformTexture("uWorldTable", this->depthToWorldTexture, 1);
					worldShader.setUniform2f("uDimensions", ofVec2f(this->depthTexture.getWidth(), this->depthTexture.getHeight()));
					worldShader.setUniform1f("uMaxDisparity", this->parameters.draw.gpuPointCloud.maxDisparity);
					worldShader.setUniformTexture("uDepthTexture", this->depthTexture, 2);

					ofPushStyle();
					{
						if (pointCloudStyle.applyIndexColor) {
							ofSetColor(this->getSubscriberColor());
						}

						if (pointCloudStyle.applyIR) {
							worldShader.setUniformTexture("uColorTexture", this->irTexture, 3);
							worldShader.setUniform1f("uColorScale", this->parameters.draw.IRAmplitude);
							worldShader.setUniform1i("uUseColorTexture", 1);
						}
						else {
							worldShader.setUniform1i("uUseColorTexture", 0);
							worldShader.setUniform1f("uColorScale", 1.0f);
						}

						this->meshProvider.getMesh().draw(OF_MESH_FILL);
					}
					ofPopStyle();
				}
				worldShader.end();
			}

			//----------
			void Subscriber::depthToWorldTableFileCallback(string & filename) {
				if (!filename.empty()) {
					try {
						this->loadDepthToWorldTableFile();
					}
					RULR_CATCH_ALL_TO_ERROR;
				}
			}

			//----------
			void Subscriber::loadDepthToWorldTableFile() {
				//load the file
				{
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
				
				//load the texture and setup shader
				{
					this->depthToWorldTexture.loadData(this->depthToWorldLUT);
				}
			}

			//----------
			ofShader & Subscriber::getWorldShader() {
				return ofxAssets::shader("ofxRulr::Nodes::MultiTrack::depthToWorld");
			}
		}
	}
}
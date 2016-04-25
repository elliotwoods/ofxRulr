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

				this->subscriber = make_shared<ofxMultiTrack::Subscriber>();

				static int colorCounter = 0;
				this->debugColor.setHsb(ofRandom(0.05f) + (0.15f * colorCounter++), 1.0f, 1.0f);
				this->worldShader = ofxAssets::shader("ofxRulr::Nodes::MultiTrack::depthToWorld");
			}

			//----------
			void Subscriber::update() {
				if (this->depthToWorldLUT.isAllocated() && !this->depthToWorldTexture.isAllocated()) {
					// TODO: Probably need a listener whenever the table changes.
					this->depthToWorldTexture.loadData(this->depthToWorldLUT);

					//Upload LUT to shader.
					this->worldShader.begin();
					{
						this->worldShader.setUniformTexture("uWorldTable", this->depthToWorldTexture, 2);
					}
					this->worldShader.end();
				}

				if (this->subscriber) {
					if (this->subscriber->getSubscriber().getAddress().compare(this->parameters.connection.publisherAddress) != 0 ||
						this->subscriber->getSubscriber().getPort() != this->parameters.connection.publisherPort) {
						// Address or Port value changed, reinitialize.
						this->subscriber->init(this->parameters.connection.publisherAddress, this->parameters.connection.publisherPort);
					}

					this->subscriber->update();

					if (this->subscriber->getSubscriber().isFrameNew()) {
						auto & pixels = this->subscriber->getFrame().getDepth();
						if (!this->previewTexture.isAllocated()) {
							this->previewTexture.allocate(pixels);
						}
						this->previewTexture.loadData(pixels);

						//Mesh update.
						if (this->parameters.draw.gpuPointCloud.enabled) {
							auto & meshDimensions = this->meshProvider.getDimensions();
							if (meshDimensions.x != pixels.getWidth() || meshDimensions.y != pixels.getHeight()) {
								this->meshProvider.setDimensions(ofVec2f(pixels.getWidth(), pixels.getHeight()));
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
				auto & frame = this->subscriber->getFrame();

				ofPushStyle();
				{
					ofSetColor(this->debugColor);

					if (this->parameters.draw.bodies) {
						const auto & bodies = frame.getBodies();
						for (const auto & body : bodies) {
							body.drawWorld(); // actually this is in kinect camera space
						}
					}

					if (this->parameters.draw.gpuPointCloud.enabled) {
						this->worldShader.begin();
						{
							this->worldShader.setUniform2f("uDimensions", ofVec2f(this->previewTexture.getWidth(), this->previewTexture.getHeight()));
							this->worldShader.setUniformTexture("uDepthTexture", this->previewTexture, 1);

							this->meshProvider.getMesh().draw(OF_MESH_POINTS);
						}
						this->worldShader.end();
					}
				}
				ofPopStyle();

				if (this->parameters.draw.cpuPointCloud.enabled) {
					this->drawPointCloud();
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
			const ofTexture & Subscriber::getPreviewTexture() const {
				return this->previewTexture;
			}

			//----------
			const ofFloatColor & Subscriber::getDebugColor() const {
				return this->debugColor;
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

			//----------
			void Subscriber::drawPointCloud() {
				if (this->depthToWorldLUT.isAllocated()) {
					const auto & frame = this->subscriber->getFrame();

					vector<ofVec3f> vertices;
					vector<ofFloatColor> colors;

					auto depth = frame.getDepth().getData();

					auto useIR = this->parameters.draw.cpuPointCloud.applyIRTexture.get() && frame.getInfrared().isAllocated();
					auto IR = frame.getInfrared().getData();
					auto depthToXY = this->depthToWorldLUT.getData();

					auto colorAmplitude = this->parameters.draw.cpuPointCloud.colorAmplitude;

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
		}
	}
}
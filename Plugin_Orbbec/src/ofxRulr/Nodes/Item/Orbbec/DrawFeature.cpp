#include "pch_Plugin_Orbbec.h"

#include "DrawFeature.h"

#include "Device.h"
#include "ofxRulr/Nodes/Item/Projector.h"
#include "ofxRulr/Nodes/System/VideoOutput.h"

namespace ofxRulr {
	namespace Nodes {
		namespace Item {
			namespace Orbbec {
				//----------
				DrawFeature::DrawFeature() {
					RULR_NODE_INIT_LISTENER;
				}

				//----------
				string DrawFeature::getTypeName() const {
					return "Item::Orbbec::DrawFeature";
				}

				//----------
				void DrawFeature::init() {
					RULR_NODE_UPDATE_LISTENER;
					this->addInput<Device>();
					this->addInput<Item::Projector>();
					auto videoOutputPin = this->addInput<System::VideoOutput>("Preview");
					
					videoOutputPin->onNewConnection += [this](shared_ptr<System::VideoOutput> videoOutput) {
						videoOutput->onDrawOutput.addListener([this](ofRectangle & bounds) {
							this->fbo.draw(bounds);
						}, this);
					};

					videoOutputPin->onDeleteConnection += [this](shared_ptr<System::VideoOutput> videoOutput) {
						if (videoOutput) {
							videoOutput->onDrawOutput.removeListeners(this);
						}
					};

					auto panel = ofxCvGui::Panels::Groups::makeStrip();
					panel->add(ofxCvGui::Panels::makeBaseDraws(this->fbo));
					panel->add(ofxCvGui::Panels::makeTexture(this->labelProbability));
					this->panel = panel;

					this->manageParameters(this->parameters);

					this->onDrawWorldStage += [this]() {
						if (this->parameters.drawToWorld) {
							this->drawFeature();
						}
					};
				}

				//----------
				void DrawFeature::update() {
					auto projectorNode = this->getInput<Item::Projector>();
					auto deviceNode = this->getInput<Device>();

					if (this->receiver.currentPort != this->parameters.oscReceiver.port) {
						this->receiver.currentPort = this->parameters.oscReceiver.port;

						this->receiver.osc = make_unique<ofxOscReceiver>();
						this->receiver.osc->setup(this->parameters.oscReceiver.port);
					}
					if (this->sender.currentPort != this->parameters.oscTarget.port || this->sender.currentAddress != this->parameters.oscTarget.address.get()) {
						this->sender.currentAddress = this->parameters.oscTarget.address;
						this->sender.currentPort = this->parameters.oscTarget.port;

						this->sender.osc = make_unique<ofxOscSender>();
						this->sender.osc->setup(this->parameters.oscTarget.address, this->parameters.oscTarget.port);
					}

					while (this->receiver.osc->hasWaitingMessages()) {
						ofxOscMessage message;
						this->receiver.osc->getNextMessage(message);
						if (message.getAddress() == "/labelIndex") {
							auto labelIndex = max((int) message.getArgAsFloat(0), message.getArgAsInt(0));
							this->parameters.draw.labelProbability.labelIndex = labelIndex;
						}
					}

					if (this->parameters.oscTarget.enabled && projectorNode && deviceNode) {
						auto device = deviceNode->getDevice();
						if (device) {
							auto skeletonStream = device->getSkeleton();
							auto projectorView = projectorNode->getViewInWorldSpace();
							if (skeletonStream) {
								auto joints = skeletonStream->getJointsRaw();
								for (const auto & joint : joints) {
									ofxOscMessage message;
									message.setAddress("/joints/" + ofToString((int)joint.first));
									auto position = ofxOrbbec::toOf(joint.second.world_position);

									position = (position / 1000.0f) * deviceNode->getTransform();
									message.addFloatArg(position.x);
									message.addFloatArg(position.y);
									message.addFloatArg(position.z);

									auto projectorPosition = projectorView.getScreenCoordinateOfWorldPosition(position);
									message.addFloatArg(projectorPosition.x);
									message.addFloatArg(projectorPosition.y);

									this->sender.osc->sendMessage(message);
								}
							}
						}
					}
					

					if (projectorNode && deviceNode) {
						auto device = deviceNode->getDevice();

						if (device) {

							if (this->fbo.getWidth() != projectorNode->getWidth() || this->fbo.getHeight() != projectorNode->getHeight()) {
								ofFbo::Settings fboSettings;
								fboSettings.width = projectorNode->getWidth();
								fboSettings.height = projectorNode->getHeight();
								fboSettings.useDepth = true;
								this->fbo.allocate(projectorNode->getWidth(), projectorNode->getHeight());
							}

							fbo.begin();
							{
								ofClear(0, 0);
								projectorNode->getViewInWorldSpace().beginAsCamera(true);
								{
									ofEnableDepthTest();
									{
										this->drawFeature();
									}
									ofDisableDepthTest();
								}
								projectorNode->getViewInWorldSpace().endAsCamera();
							}
							fbo.end();
						}
					}

					if (this->spoutSender && !this->parameters.spout.enabled) {
						this->spoutSender.reset();
					}
					else if (!this->spoutSender && this->parameters.spout.enabled) {
						this->spoutSender = make_unique<ofxSpout::Sender>();
						this->spoutSender->init(this->parameters.spout.channel.get());
					}
					if (this->spoutSender) {
						if (this->spoutSender->getChannelName() != this->parameters.spout.channel.get()) {
							this->spoutSender->init(this->parameters.spout.channel.get());
						}
						if (this->fbo.isAllocated()) {
							this->spoutSender->send(this->fbo.getTexture());
						}
					}
				}

				//----------
				void DrawFeature::drawFeature() {
					auto deviceNode = this->getInput<Device>();
					auto device = deviceNode->getDevice();

					if (this->parameters.draw.mesh) {
						auto points = device->getPoints();
						if (points) {
							ofPushMatrix();
							{
								auto drawLabels = this->parameters.draw.labelProbability.enabled.get() && device->has<ofxOrbbec::Streams::Skeleton>();
								if (drawLabels) {
									if (this->parameters.draw.labelProbability.labelIndex == 0) {
										this->labelProbability.loadData(device->getSkeleton()->getUserMask());
									}
									else {
										this->labelProbability.loadData(device->getSkeleton()->getProbabilityMap(this->parameters.draw.labelProbability.labelIndex));
									}
									this->labelProbability.bind();
									ofSetMatrixMode(ofMatrixMode::OF_MATRIX_TEXTURE);
									ofPushMatrix();
									ofScale(this->labelProbability.getWidth() / points->getWidth(), this->labelProbability.getHeight() / points->getHeight());
									ofSetMatrixMode(ofMatrixMode::OF_MATRIX_MODELVIEW);
								}

								ofMultMatrix(deviceNode->getTransform());
								ofScale(0.001f, -0.001f, 0.001f);
								points->getMesh().drawFaces();

								if (drawLabels) {
									ofSetMatrixMode(ofMatrixMode::OF_MATRIX_TEXTURE);
									ofPopMatrix();
									ofSetMatrixMode(ofMatrixMode::OF_MATRIX_MODELVIEW);
									this->labelProbability.unbind();
								}
							}
							ofPopMatrix();
						}
					}

					if (this->parameters.draw.skeleton) {
						auto skeleton = device->getSkeleton();
						if (skeleton) {
							skeleton->drawSkeleton3D();
						}
					}
				}

				//----------
				ofxCvGui::PanelPtr DrawFeature::getPanel() {
					return this->panel;
				}
			}
		}
	}
}
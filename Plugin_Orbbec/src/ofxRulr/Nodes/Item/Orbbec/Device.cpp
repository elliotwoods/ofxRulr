#include "pch_Plugin_Orbbec.h"
#include "Device.h"

namespace ofxRulr {
	namespace Nodes {
		namespace Item {
			namespace Orbbec {
				//----------
				Device::Device() {
					RULR_NODE_INIT_LISTENER;
				}

				//----------
				string Device::getTypeName() const {
					return "Item::Orbbec::Device";
				}

				//----------
				void Device::init() {
					RULR_NODE_UPDATE_LISTENER;
					RULR_RIGIDBODY_DRAW_OBJECT_LISTENER;


					this->manageParameters(this->parameters);

					this->parameters.enabledStreams.color.addListener(this, &Device::streamEnableCallback);
					this->parameters.enabledStreams.depth.addListener(this, &Device::streamEnableCallback);
					this->parameters.enabledStreams.infrared.addListener(this, &Device::streamEnableCallback);
					this->parameters.enabledStreams.points.addListener(this, &Device::streamEnableCallback);
					this->parameters.enabledStreams.skeleton.addListener(this, &Device::streamEnableCallback);

					this->device = make_shared<ofxOrbbec::Device>();
					this->device->open();

					this->panelStrip = make_shared<ofxCvGui::Panels::Groups::Strip>();
				}

				//----------
				void Device::update() {
					if (this->streamsNeedRebuilding) {
						this->rebuildStreams();
					}

					if (this->device) {
						this->device->update();
					}
				}

				//----------
				void Device::drawObject() {
					if (this->device) {
						{
							auto points = this->device->get<ofxOrbbec::Streams::Points>(false);
							if (points) {
								ofPushMatrix();
								{
									ofScale(+0.001, -0.001, 0.001);
									if (this->parameters.drawFaces) {
										points->getMesh().drawFaces();
									}
									else {
										points->getMesh().drawVertices();
									}
								}
								ofPopMatrix();
							}
						}
						{
							auto skeleton = this->device->get<ofxOrbbec::Streams::Skeleton>(false);
							if (skeleton) {
								ofPushMatrix();
								{
									ofScale(0.001, 0.001, 0.001);
									skeleton->drawSkeleton3D();
								}
								ofPopMatrix();
							}
						}
					}
				}

				//----------
				ofxCvGui::PanelPtr Device::getPanel() {
					return this->panelStrip;
				}

				//----------
				shared_ptr<ofxOrbbec::Device> Device::getDevice() {
					return this->device;
				}

				//----------
				template<typename StreamType>
				void syncStream(shared_ptr<ofxOrbbec::Device> device, bool enabled) {
					if (device->has<StreamType>()) {
						if (!enabled) {
							cout << "Closing" << typeid(StreamType).name() << endl;
							device->close<StreamType>();
						}
					}
					else {
						if (enabled) {
							device->init<StreamType>();
						}
					}
				}

				//----------
				template<typename StreamType>
				void disableMirroring(shared_ptr<ofxOrbbec::Streams::Base> stream) {
					auto typedStream = dynamic_pointer_cast<StreamType>(stream);
					if (typedStream) {
						typedStream->getStream().enable_mirroring(false);
					}
				}

				//----------
				void Device::rebuildStreams() {
					if (!this->device) {
						return;
					}

					//remove conflicts
					if (this->device->has<ofxOrbbec::Streams::Color>() && this->parameters.enabledStreams.infrared) {
						this->device->closeColor();
					}
					if (this->device->has<ofxOrbbec::Streams::Infrared>() && this->parameters.enabledStreams.color) {
						this->device->closeInfrared();
					}
					if (this->parameters.enabledStreams.color && this->parameters.enabledStreams.infrared) {
						this->parameters.enabledStreams.infrared = false;
					}
					if (!this->parameters.enabledStreams.points && this->parameters.enabledStreams.skeleton) {
						this->parameters.enabledStreams.skeleton = false;
					}
					if ((this->parameters.enabledStreams.points || this->parameters.enabledStreams.infrared) && !this->parameters.enabledStreams.depth) {
						this->parameters.enabledStreams.depth = true;
					}

					//setup streams
					syncStream<ofxOrbbec::Streams::Color>(this->device, this->parameters.enabledStreams.color);
					syncStream<ofxOrbbec::Streams::Depth>(this->device, this->parameters.enabledStreams.depth);
					syncStream<ofxOrbbec::Streams::Infrared>(this->device, this->parameters.enabledStreams.infrared);
					syncStream<ofxOrbbec::Streams::Points>(this->device, this->parameters.enabledStreams.points);
					syncStream<ofxOrbbec::Streams::Skeleton>(this->device, this->parameters.enabledStreams.skeleton);

					for (auto stream : this->device->getStreams()) {
						disableMirroring<ofxOrbbec::Streams::Color>(stream);
						disableMirroring<ofxOrbbec::Streams::Depth>(stream);
						disableMirroring<ofxOrbbec::Streams::Infrared>(stream);
					}

					//build gui
					this->panelStrip->clear();
					for (auto stream : this->device->getStreams()) {
						{
							auto depthStream = dynamic_pointer_cast<ofxOrbbec::Streams::Depth>(stream);
							if (depthStream) {
								auto panel = ofxCvGui::Panels::makeTexture(depthStream->getTexture(), depthStream->getTypeName());
								auto style = make_shared<ofxCvGui::Panels::Texture::Style>();
								style->rangeMinimum = 0.0f;
								style->rangeMaximum = 5000.0f / float(1 << 16);
								panel->setStyle(style);
								this->panelStrip->add(panel);
								continue;
							}
						}

						{
							auto infraredStream = dynamic_pointer_cast<ofxOrbbec::Streams::Infrared>(stream);
							if (infraredStream) {
								auto panel = ofxCvGui::Panels::makeTexture(infraredStream->getTexture(), infraredStream->getTypeName());
								auto style = make_shared<ofxCvGui::Panels::Texture::Style>();
								style->rangeMinimum = 0.0f;
								style->rangeMaximum = 1000 / float(1 << 16);
								panel->setStyle(style);
								this->panelStrip->add(panel);
								continue;
							}
						}

						{
							auto imageStream = dynamic_pointer_cast<ofxOrbbec::Streams::BaseImage>(stream);
							if (imageStream) {
								auto panel = ofxCvGui::Panels::makeBaseDraws(*imageStream, imageStream->getTypeName());
								this->panelStrip->add(panel);
								continue;
							}
						}
					}

					this->streamsNeedRebuilding = false;
				}

				//----------
				void Device::streamEnableCallback(bool &) {
					this->streamsNeedRebuilding = true;
				}
			}
		}
	}
}
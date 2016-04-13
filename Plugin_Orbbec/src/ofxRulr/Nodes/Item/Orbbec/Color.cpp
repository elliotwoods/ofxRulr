#include "pch_Plugin_Orbbec.h"
#include "Color.h"

namespace ofxRulr {
	namespace Nodes {
		namespace Item {
			namespace Orbbec {
				//----------
				Color::Color() {
					RULR_NODE_INIT_LISTENER;
				}

				//----------
				string Color::getTypeName() const {
					return "Item::Orbbec::Color";
				}

				//----------
				void Color::init() {
					RULR_NODE_UPDATE_LISTENER;

					this->manageParameters(this->parameters);

					this->addInput<Device>();
					this->panelStrip = ofxCvGui::Panels::Groups::makeStrip();
					this->panelStrip->add(ofxCvGui::Panels::makeTexture(this->texture));

					this->setWidth(640);
					this->setHeight(480);
					this->focalLengthX = 570;
					this->focalLengthY = 570;
					this->principalPointX = 320;
					this->principalPointY = 240;

					this->onDrawObject += [this]() {
						if (this->texture.isAllocated()) {
							this->getViewInObjectSpace().drawOnNearPlane(this->texture);
						}
					};
				}

				//----------
				void Color::update() {
					auto deviceNode = this->getInput<Device>();
					if (deviceNode) {
						auto device = deviceNode->getDevice();
						if (device) {
							if (device->isFrameNew()) {
								auto colorStream = device->get<ofxOrbbec::Streams::Color>(false);
								if (colorStream) {
									this->texture = colorStream->getTexture();
								}
							}
						}


						if (this->parameters.renderDepthMap) {
							try {
								this->renderDepthMap();
							}
							RULR_CATCH_ALL_TO_ERROR;
						}
					}
				}

				//----------
				ofxCvGui::PanelPtr Color::getPanel() {
					return this->panelStrip;
				}

				//----------
				ofVec3f Color::cameraToDepth(const ofVec2f & camera) {
					if (!this->parameters.renderDepthMap || !ofRectangle(0, 0, this->colorDepthBufferPixels.getWidth(), this->colorDepthBufferPixels.getHeight()).inside(camera)) {
						return ofVec3f();
					}

					if (this->colorDepthBufferPixels.isAllocated()) {
						auto depthValue = this->colorDepthBufferPixels[floor(camera.x) + floor(camera.y) * this->colorDepthBufferPixels.getWidth()];
						if (depthValue == 1.0f) {
							depthValue = 0.0f; // far plane = nothing found
						}
						return ofVec3f(camera.x, camera.y, depthValue);
					}
					else {
						return ofVec3f();
					}
				}

				//----------
				ofVec3f Color::cameraToWorld(const ofVec2f & camera) {
					auto screenDepth = this->cameraToDepth(camera);
					if (screenDepth.z == 0) {
						return ofVec3f();
					}

					auto clipCoord = screenDepth;
					clipCoord.x = 2 * (clipCoord.x / 640.0f) - 1.0f;
					clipCoord.y = -(2 * (clipCoord.y / 480.0f) - 1.0f);
					clipCoord.z = clipCoord.z * 2.0f - 1.0f;

					auto viewProjection = this->getViewInWorldSpace().getViewMatrix() * this->getViewInWorldSpace().getClippedProjectionMatrix();
					return clipCoord * viewProjection.getInverse();
				}

				//----------
				void Color::renderDepthMap() {
					ofFbo fbo;
					try {
						this->throwIfMissingAConnection<Item::Orbbec::Device>();

						auto deviceNode = this->getInput<Item::Orbbec::Device>();

						auto device = deviceNode->getDevice();
						if (!device) {
							throw(ofxRulr::Exception("Device not initialised"));
						}
						auto pointStream = deviceNode->getDevice()->getPoints();
						if (!pointStream) {
							throw(ofxRulr::Exception("Point stream not available"));
						}

						if (!this->drawPointCloud.isAllocated()) {
							ofFbo::Settings fboSettings;
							fboSettings.width = this->getWidth();
							fboSettings.height = this->getHeight();
							fboSettings.useDepth = true;
							fboSettings.depthStencilInternalFormat = GL_DEPTH_COMPONENT32;
							fboSettings.depthStencilAsTexture = true;

							this->drawPointCloud.allocate(fboSettings);

							fboSettings.useDepth = false;
							fboSettings.depthStencilAsTexture = false;
							fboSettings.internalformat = GL_R32F;

							this->extractDepthBuffer.allocate(fboSettings);

							{
								{
									auto depthPreview = make_shared<ofxCvGui::Panels::Texture>(this->drawPointCloud.getDepthTexture());
									depthPreview->setCaption("Depth from color view");
									this->panelStrip->add(depthPreview);
								}
							}
						}

						auto & colorView = this->getViewInWorldSpace();

						this->drawPointCloud.begin();
						ofEnableDepthTest();
						ofClear(0, 255);
						{
							colorView.beginAsCamera(true);
							{
								ofPushMatrix();
								{
									deviceNode->drawWorld();
								}
								ofPopMatrix();
							}
							colorView.endAsCamera();
						}
						ofDisableDepthTest();
						this->drawPointCloud.end();

						this->extractDepthBuffer.begin();
						this->drawPointCloud.getDepthTexture().draw(0, 0);
						this->extractDepthBuffer.end();

						this->extractDepthBuffer.getTexture().readToPixels(colorDepthBufferPixels);
					}
					RULR_CATCH_ALL_TO_ERROR;
				}
			}
		}
	}
}
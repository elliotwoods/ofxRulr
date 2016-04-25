#include "pch_MultiTrack.h"
#include "ImageBox.h"

#include "World.h"

namespace ofxRulr {
	namespace Nodes {
		namespace MultiTrack {
			//----------
			ImageBox::ImageBox() {
				RULR_NODE_INIT_LISTENER;
			}

			//----------
			string ImageBox::getTypeName() const {
				return "MultiTrack::ImageBox";
			}

			//----------
			void ImageBox::init() {
				RULR_NODE_UPDATE_LISTENER;
				RULR_RIGIDBODY_DRAW_OBJECT_LISTENER;

				this->addInput<World>();
				this->manageParameters(this->parameters);

				this->panel = ofxCvGui::Panels::Groups::makeStrip();
			}

			//----------
			void ImageBox::update() {
				if (this->fbo.getWidth() != this->parameters.resolution.width
					|| this->fbo.getHeight() != this->parameters.resolution.height) {
					ofFbo::Settings fboSettings;
					fboSettings.width = this->parameters.resolution.width;
					fboSettings.height = this->parameters.resolution.height;
					fboSettings.useDepth = true;
					fboSettings.depthStencilAsTexture = true;
					fboSettings.internalformat = GL_RGBA;
					this->fbo.allocate(fboSettings);

					this->panel->clear();
					this->panel->add(ofxCvGui::Panels::makeTexture(this->fbo.getTexture(), "Texture"));
					this->panel->add(ofxCvGui::Panels::makeTexture(this->fbo.getDepthTexture(), "Depth"));
				}
				if (this->fbo.isAllocated()) {
					auto world = this->getInput<World>();
					if (world) {
						this->fbo.begin(false);
						ofClear(0, 0);
						ofEnableDepthTest();
						{
							ofPushMatrix();
							{
								ofSetMatrixMode(ofMatrixMode::OF_MATRIX_PROJECTION);
								ofLoadIdentityMatrix();
								ofSetMatrixMode(ofMatrixMode::OF_MATRIX_MODELVIEW);
								ofLoadIdentityMatrix();
								ofScale(1.0f, -1.0f, 1.0f);

								ofScale(2.0f / this->parameters.box.width, 2.0f / this->parameters.box.height, 2.0f / this->parameters.box.depth);
								ofMultMatrix(this->getTransform().getInverse());
								
								auto subscribers = world->getSubscribers();
								for (auto subscriberIterator : subscribers) {
									auto subscriber = subscriberIterator.second.lock();
									if (subscriber) {
										Subscriber::PointCloudStyle style;
										style.applyIndexColor = this->parameters.drawStyle.subscriberIndex;
										style.applyIR = this->parameters.drawStyle.IR;
										subscriber->drawPointCloudGpu(style);
									}
								}
							}
							ofPopMatrix();
						}
						ofDisableDepthTest();
						this->fbo.end();
					}
				}
			}

			//----------
			void ImageBox::drawObject() {
				ofPushStyle();
				{
					ofNoFill();
					ofSetColor(this->getColor());
					ofDrawBox(ofVec3f(), this->parameters.box.width, this->parameters.box.height, this->parameters.box.depth);
				}
				ofPopStyle();
			}

			//----------
			ofxCvGui::PanelPtr ImageBox::getPanel() {
				return this->panel;
			}
		}
	}
}
#include "pch_Plugin_Orbbec.h"
#include "Orbbec.h"

namespace ofxRulr {
	namespace Nodes {
		namespace Item {
			//----------
			Orbbec::Orbbec() {
				RULR_NODE_INIT_LISTENER;
			}

			//----------
			string Orbbec::getTypeName() const {
				return "Nodes::Item::Orbbec";
			}

			//----------
			void Orbbec::init() {
				RULR_NODE_UPDATE_LISTENER;
				RULR_RIGIDBODY_DRAW_OBJECT_LISTENER;
				

				this->device = make_shared<ofxOrbbec::Device>();
				this->device->open();
				//this->device->initColor();
				this->device->initInfrared();
				this->device->initDepth();
				this->device->initPoints();

				this->panelStrip = make_shared<ofxCvGui::Panels::Groups::Strip>();

				for (auto stream : this->device->getStreams()) {
					{
						auto depthStream = dynamic_pointer_cast<ofxOrbbec::Streams::Depth>(stream);
						if (depthStream) {
							auto panel = ofxCvGui::Panels::makeTexture(depthStream->getTexture(), depthStream->getTypeName());
							auto style = make_shared<ofxCvGui::Panels::Texture::Style>();
							style->rangeMinimum = 0.0f;
							style->rangeMaximum = 8000.0f;
							panel->setStyle(style);
						}
					}

					{
						auto infraredStream = dynamic_pointer_cast<ofxOrbbec::Streams::Infrared>(stream);
						if (infraredStream) {
							auto panel = ofxCvGui::Panels::makeTexture(infraredStream->getTexture(), infraredStream->getTypeName());
							auto style = make_shared<ofxCvGui::Panels::Texture::Style>();
							style->rangeMinimum = 0.0f;
							style->rangeMaximum = 1 << 12;
							panel->setStyle(style);
						}
					}

					{
						auto imageStream = dynamic_pointer_cast<ofxOrbbec::Streams::BaseImage>(stream);
						if(imageStream) {
							auto panel = ofxCvGui::Panels::makeBaseDraws(*imageStream, imageStream->getTypeName());
							this->panelStrip->add(panel);
							continue;
						}
					}
				}
			}

			//----------
			void Orbbec::update() {
				if (this->device) {
					this->device->update();
				}
			}

			//----------
			void Orbbec::drawObject() {
				if (this->device) {
					ofPushMatrix();
					{
						ofScale(0.001, 0.001, 0.001);
						this->device->getPoints()->getMesh().drawVertices();
					}
					ofPopMatrix();
				}
			}

			//----------
			ofxCvGui::PanelPtr Orbbec::getPanel() {
				return this->panelStrip;
			}
		}
	}
}
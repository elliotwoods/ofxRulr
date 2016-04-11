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

					this->addInput<Device>();
					this->panel = ofxCvGui::Panels::makeTexture(this->texture);

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
					}
				}

				//----------
				ofxCvGui::PanelPtr Color::getPanel() {
					return this->panel;
				}
			}
		}
	}
}
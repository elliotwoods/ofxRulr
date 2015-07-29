#include "KinectV2.h"

#include "ofxCvGui/Panels/Draws.h"
#include "ofxCvGui/Widgets/MultipleChoice.h"

using namespace ofxCvGui;

namespace ofxRulr {
	namespace Nodes {
		namespace Item {
			//----------
			KinectV2::KinectV2() {
				this->onInit += [this]() {
					this->init();
				};
				//RULR_NODE_INIT_LISTENER;
			}

			//----------
			void KinectV2::init() {
				RULR_NODE_UPDATE_LISTENER;
				RULR_NODE_SERIALIZATION_LISTENERS;
				RULR_NODE_INSPECTOR_LISTENER;

				auto view = MAKE(ofxCvGui::Panels::Groups::Grid);
				this->view = view;

				this->device = MAKE(ofxKinectForWindows2::Device);
				this->device->open();

				if (this->device->isOpen()) {
					this->device->initDepthSource();
					this->device->initColorSource();
					this->device->initBodySource();

					auto rgbView = make_shared<ofxCvGui::Panels::Draws>(this->device->getColorSource()->getTexture());
					auto depthView = make_shared<ofxCvGui::Panels::Draws>(this->device->getDepthSource()->getTexture());
					rgbView->setCaption("RGB");
					depthView->setCaption("Depth");
					this->view->add(rgbView);
					this->view->add(depthView);
				}
				else {
					throw(Exception("Cannot initialise Kinect device. We should find a way to fail elegantly here (and retry later)."));
				}

				this->playState.set("Play state", 0, 0, 1);
				this->viewType.set("View type", 3, 0, 3);
			}

			//----------
			string KinectV2::getTypeName() const {
				return "Item::KinectV2";
			}

			//----------
			void KinectV2::update() {
				if (this->device && this->playState == 0) {
					this->device->update();
				}
			}

			//----------
			ofxCvGui::PanelPtr KinectV2::getView() {
				return this->view;
			}

			//----------
			void KinectV2::serialize(Json::Value & json) {
				ofxRulr::Utils::Serializable::serialize(this->viewType, json);
			}

			//----------
			void KinectV2::deserialize(const Json::Value & json) {
				ofxRulr::Utils::Serializable::deserialize(this->viewType, json);
			}

			//----------
			void KinectV2::drawObject() {
				if (this->device) {
					switch (this->viewType.get()) { // don't break on the cases, flow through
					case 2:
						//this should be something like 'draw pretty mesh'
						//something seems to have been missed out of a merge in ofxKinectForWindows2
						this->device->drawWorld();
					case 1:
					{
						auto bodySource = this->device->getBodySource();
						if (bodySource) {
							bodySource->drawWorld();
						}
					}
					case 0:
					{
						ofPushStyle();
						ofSetColor(this->getColor());
						ofNoFill();
						ofSetLineWidth(1.0f);
						auto depthSource = this->device->getDepthSource();
						if (depthSource) {
							depthSource->drawFrustum();
						}
						auto colorSource = this->device->getColorSource();
						if (colorSource) {
							colorSource->drawFrustum();
						}
						ofPopStyle();
					}
					default:
						break;
					}
				}
			}

			//----------
			shared_ptr<ofxKinectForWindows2::Device> KinectV2::getDevice() {
				return this->device;
			}

			//----------
			void KinectV2::populateInspector(ofxCvGui::ElementGroupPtr inspector) {
				auto selectPlayState = make_shared<ofxCvGui::Widgets::MultipleChoice>("Play state");
				selectPlayState->addOption("Play");
				selectPlayState->addOption("Pause");
				selectPlayState->entangle(this->playState);
				inspector->add(selectPlayState);

				auto selectViewType = make_shared<ofxCvGui::Widgets::MultipleChoice>("3D View");
				selectViewType->addOption("Frustums");
				selectViewType->addOption("Bodies");
				selectViewType->addOption("All");
				selectViewType->entangle(this->viewType);
				inspector->add(selectViewType);
			}
		}
	}
}
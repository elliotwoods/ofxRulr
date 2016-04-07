#include "pch_Plugin_KinectForWindows2.h"
#include "KinectV2.h"

#include "ofxCvGui/Panels/Texture.h"
#include "ofxCvGui/Widgets/MultipleChoice.h"

using namespace ofxCvGui;

namespace ofxRulr {
	namespace Nodes {
		namespace Item {
			//----------
			KinectV2::KinectV2() {
				RULR_NODE_INIT_LISTENER;
			}

			//----------
			void KinectV2::init() {
				RULR_NODE_UPDATE_LISTENER;
				RULR_RIGIDBODY_DRAW_OBJECT_LISTENER;
				RULR_NODE_SERIALIZATION_LISTENERS;
				RULR_NODE_INSPECTOR_LISTENER;

				auto view = MAKE(ofxCvGui::Panels::Groups::Strip);
				this->view = view;

				this->device = MAKE(ofxKinectForWindows2::Device);
				this->device->open();

				if (this->device->isOpen()) {
					this->device->initColorSource();
					this->device->initDepthSource();
					this->device->initInfraredSource();
					this->device->initBodySource();
					this->device->initBodyIndexSource();
				}
				else {
					throw(Exception("Cannot initialise Kinect device. We should find a way to fail elegantly here (and retry later)."));
				}

				this->playState.set("Play state", 0, 0, 1);
				this->viewType.set("View type", 3, 0, 3);

				{
					this->enabledViews.rgb.set("RGB", true);
					this->enabledViews.depth.set("Depth", true);
					this->enabledViews.ir.set("IR", false);
					this->enabledViews.body.set("Body", false);
				}
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
			ofxCvGui::PanelPtr KinectV2::getPanel() {
				return this->view;
			}

			//----------
			void KinectV2::serialize(Json::Value & json) {
				ofxRulr::Utils::Serializable::serialize(this->viewType, json);
				auto & jsonEnabledViews = json["enabledViews"];
				{
					ofxRulr::Utils::Serializable::serialize(this->enabledViews.rgb, jsonEnabledViews);
					ofxRulr::Utils::Serializable::serialize(this->enabledViews.depth, jsonEnabledViews);
					ofxRulr::Utils::Serializable::serialize(this->enabledViews.ir, jsonEnabledViews);
					ofxRulr::Utils::Serializable::serialize(this->enabledViews.body, jsonEnabledViews);
				}
			}

			//----------
			void KinectV2::deserialize(const Json::Value & json) {
				ofxRulr::Utils::Serializable::deserialize(this->viewType, json);
				const auto & jsonEnabledViews = json["enabledViews"];
				{
					ofxRulr::Utils::Serializable::deserialize(this->enabledViews.rgb, jsonEnabledViews);
					ofxRulr::Utils::Serializable::deserialize(this->enabledViews.depth, jsonEnabledViews);
					ofxRulr::Utils::Serializable::deserialize(this->enabledViews.ir, jsonEnabledViews);
					ofxRulr::Utils::Serializable::deserialize(this->enabledViews.body, jsonEnabledViews);
				}
				this->rebuildView();
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
			void KinectV2::populateInspector(ofxCvGui::InspectArguments & inspectArgs) {
				auto inspector = inspectArgs.inspector;

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

				inspector->add(new Widgets::Title("Enabled views", Widgets::Title::Level::H2));
				{
					auto changeViewsCallback = [this](ofParameter<bool> &) {
						this->rebuildView();
					};
					inspector->addToggle(this->enabledViews.rgb)->onValueChange += changeViewsCallback;
					inspector->addToggle(this->enabledViews.depth)->onValueChange += changeViewsCallback;
					inspector->addToggle(this->enabledViews.ir)->onValueChange += changeViewsCallback;
					inspector->addToggle(this->enabledViews.body)->onValueChange += changeViewsCallback;
				}
			}

			//----------
			void KinectV2::rebuildView() {
				this->view->clear();

				if(this->enabledViews.rgb) {
					auto rgbView = make_shared<ofxCvGui::Panels::Texture>(this->device->getColorSource()->getTexture());
					rgbView->setCaption("RGB");
					this->view->add(rgbView);
				}

				if (this->enabledViews.depth) {
					auto depthView = make_shared<ofxCvGui::Panels::Texture>(this->device->getDepthSource()->getTexture());
					depthView->setCaption("Depth");
					
					auto style = Panels::Texture::Style();
					style.rangeMaximum = 16384.0f / pow(2.0f, 16); // range is drawn as 0...16384mm (0..0.25 pixel value)
					//depthView->setStyle(style); //can't use shader because we're a dll

					this->view->add(depthView);
				}

				if (this->enabledViews.ir) {
					auto irView = make_shared<ofxCvGui::Panels::Texture>(this->device->getInfraredSource()->getTexture());
					irView->setCaption("IR");
					this->view->add(irView);
				}

				if (this->enabledViews.body) {
					auto & texture = this->device->getBodyIndexSource()->getTexture();
					auto bodyView = make_shared<ofxCvGui::Panels::Texture>(texture);
					bodyView->setCaption("Body");
					
					auto style = Panels::Texture::Style();
					style.rangeMaximum = 5.0f;
					//bodyView->setStyle(style); //can't use shader because we're a dll

					auto width = texture.getWidth();
					auto height = texture.getHeight();
					bodyView->onDrawImage += [this, width, height](DrawImageArguments & args) {
						auto bodySource = this->device->getBodySource();
						bodySource->drawProjected(0, 0, width, height, ofxKinectForWindows2::ProjectionCoordinates::DepthCamera);
					};

					this->view->add(bodyView);
				}
			}
		}
	}
}

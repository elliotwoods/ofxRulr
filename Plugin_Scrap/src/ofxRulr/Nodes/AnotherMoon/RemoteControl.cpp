#include "pch_Plugin_Scrap.h"
#include "RemoteControl.h"
#include "Lasers.h"

namespace ofxRulr {
	namespace Nodes {
		namespace AnotherMoon {
			//----------
			RemoteControl::RemoteControl()
			{
				RULR_NODE_INIT_LISTENER;
			}

			//----------
			string
				RemoteControl::getTypeName() const
			{
				return "AnotherMoon::RemoteControl";
			}

			//----------
			void
				RemoteControl::init()
			{
				this->manageParameters(this->parameters);
				RULR_NODE_INSPECTOR_LISTENER;

				this->addInput<Lasers>();

				this->panel = ofxCvGui::Panels::makeBlank();
				this->panel->onDraw += [this](ofxCvGui::DrawArguments& args) {
					auto lasersNode = this->getInput<Lasers>();
					if (lasersNode) {
						auto lasers = lasersNode->getLasersSelected();
						for (auto laser : lasers) {
							auto centerOffset = laser->parameters.intrinsics.centerOffset.get();

							auto drawX = ofMap(centerOffset.x, -1, 1, 0, args.localBounds.getWidth());
							auto drawY = ofMap(centerOffset.y, 1, -1, 0, args.localBounds.getHeight());

							ofPushStyle();
							{
								ofSetColor(laser->color);
								ofDrawCircle(drawX, drawY, 10.0f);
							}
							ofPopStyle();
						}
					}
				};
				auto panelWeak = weak_ptr<ofxCvGui::Panels::Base>(this->panel);
				this->panel->onMouse += [this, panelWeak](ofxCvGui::MouseArguments& args) {
					auto panel = panelWeak.lock();
					if (args.takeMousePress(panel)) {

					}
					if (args.isDragging(panel)) {
						auto lasersNode = this->getInput<Lasers>();
						if (lasersNode) {
							auto movement = glm::vec2(
								args.movement.x / panel->getWidth()
								, -args.movement.y / panel->getHeight()
							);

							movement *= ofGetKeyPressed(OF_KEY_SHIFT)
								? this->parameters.adjust.fastSpeed.get()
								: this->parameters.adjust.slowSpeed.get();

							auto lasers = lasersNode->getLasersSelected();
							for (auto laser : lasers) {
								auto newCenter = laser->parameters.intrinsics.centerOffset.get()  + movement;
								laser->parameters.intrinsics.centerOffset.set(newCenter);
							}
						}
					}
				};
				this->panel->onKeyboard += [this](ofxCvGui::KeyboardArguments& args) {
					auto lasersNode = this->getInput<Lasers>();
					if (lasersNode) {
						glm::vec2 movement{ 0, 0 };
						switch (args.key) {
						case OF_KEY_UP:
							movement.y = 1.0f;
							break;
						case OF_KEY_DOWN:
							movement.y = -1.0f;
							break;
						case OF_KEY_LEFT:
							movement.x = -1.0f;
							break;
						case OF_KEY_RIGHT:
							movement.x = 1.0f;
							break;
						default:
							break;
						}

						movement *= 0.1f;
						movement *= ofGetKeyPressed(OF_KEY_SHIFT)
							? this->parameters.adjust.fastSpeed.get()
							: this->parameters.adjust.slowSpeed.get();

						auto lasers = lasersNode->getLasersSelected();
						for (auto laser : lasers) {
							auto newCenter = laser->parameters.intrinsics.centerOffset.get() + movement;
							laser->parameters.intrinsics.centerOffset.set(newCenter);
						}
					}
				};
			}

			//---------
			void
				RemoteControl::populateInspector(ofxCvGui::InspectArguments& args)
			{
				auto inspector = args.inspector;

				inspector->addButton("Home all", [this]() {
					try {
						this->homeAll();
					}
					RULR_CATCH_ALL_TO_ALERT;
					});
				inspector->addButton("Set RGB all", [this]() {
					try {
						this->setRGBAll(
							this->parameters.setValues.color.red
							, this->parameters.setValues.color.green
							, this->parameters.setValues.color.blue
						);
					}
					RULR_CATCH_ALL_TO_ALERT;
					}, ' ');

				inspector->addButton("Set Transform all", [this]() {
					try {
						this->setTransformAll(
							this->parameters.setValues.transform.sizeX
							, this->parameters.setValues.transform.sizeY
							, this->parameters.setValues.transform.offsetX
							, this->parameters.setValues.transform.offsetY
						);
					}
					RULR_CATCH_ALL_TO_ALERT;
					}, 't');

			}

			//---------
			ofxCvGui::PanelPtr
				RemoteControl::getPanel()
			{
				return this->panel;
			}

			//---------
			void
				RemoteControl::homeAll()
			{
				this->throwIfMissingAnyConnection();
				auto lasersNode = this->getInput<Lasers>();
				auto lasers = lasersNode->getLasersSelected();
				for (auto laser : lasers) {
					laser->parameters.intrinsics.centerOffset.set({ 0, 0 });
				}
			}

			//---------
			void
				RemoteControl::setRGBAll(float red, float green, float blue)
			{
				this->throwIfMissingAnyConnection();
				auto lasersNode = this->getInput<Lasers>();
				auto lasers = lasersNode->getLasersSelected();
				for (auto laser : lasers) {
					laser->parameters.deviceState.projection.color.red.set(red);
					laser->parameters.deviceState.projection.color.green.set(green);
					laser->parameters.deviceState.projection.color.blue.set(blue);
				}
			}

			//---------
			void
				RemoteControl::setTransformAll(float sizeX, float sizeY, float offsetX, float offsetY)
			{
				this->throwIfMissingAnyConnection();
				auto lasersNode = this->getInput<Lasers>();
				auto lasers = lasersNode->getLasersSelected();
				for (auto laser : lasers) {
					laser->parameters.deviceState.projection.transform.sizeX.set(sizeX);
					laser->parameters.deviceState.projection.transform.sizeY.set(sizeY);
					laser->parameters.deviceState.projection.transform.offsetX.set(offsetX);
					laser->parameters.deviceState.projection.transform.offsetY.set(offsetY);
				}
			}
		}
	}
}
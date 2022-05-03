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

				this->addInput<Lasers>();

				this->panel = ofxCvGui::Panels::makeBlank();
				this->panel->onDraw += [this](ofxCvGui::DrawArguments& args) {
					auto lasersNode = this->getInput<Lasers>();
					if (lasersNode) {
						auto lasers = lasersNode->getLasersSelected();
						for (auto laser : lasers) {
							auto centerOffset = laser->parameters.settings.centerOffset.get();

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
								auto newCenter = laser->parameters.settings.centerOffset.get()  + movement;
								laser->parameters.settings.centerOffset.set(newCenter);
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
							auto newCenter = laser->parameters.settings.centerOffset.get() + movement;
							laser->parameters.settings.centerOffset.set(newCenter);
						}
					}
				};
			}

			//---------
			ofxCvGui::PanelPtr
				RemoteControl::getPanel()
			{
				return this->panel;
			}

		}
	}
}
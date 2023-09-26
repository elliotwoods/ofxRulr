#include "pch_Plugin_DualSense.h"
#include "Controller.h"

namespace ofxRulr {
	namespace Nodes {
		namespace DualSense {
			//----------
			Controller::Controller()
			{
				RULR_NODE_INIT_LISTENER;
			}

			//----------
			string
				Controller::getTypeName() const
			{
				return "DualSense::Controller";
			}

			//----------
			void
				Controller::init()
			{
				RULR_NODE_UPDATE_LISTENER;

				this->panel = ofxCvGui::Panels::Groups::makeStrip();
				this->refreshPanel();

				this->addInput<Nodes::Base>();

				this->manageParameters(this->parameters);

				if (this->parameters.index.get() >= 0 && !this->controller) {
					this->setDeviceIndex(this->parameters.index.get());
				}
			}

			//----------
			void
				Controller::update()
			{
				if (this->controller) {
					// we have a controller
					this->controller->update();
					this->priorInputState = this->inputState;
					this->inputState = this->controller->getInputState();

					auto input = this->getInput<Nodes::Base>();
					if (input) {
						// we have a target node
						ofxRulr::RemoteControllerArgs args;
						{
							args.next = this->inputState.DPad.x > 0 && !this->priorInputState.DPad.x > 0
								|| this->inputState.buttons.rightBumper && !this->priorInputState.buttons.rightBumper;

							args.previous = this->inputState.DPad.x < 0 && !this->priorInputState.DPad.x < 0
								|| this->inputState.buttons.leftBumper && !this->priorInputState.buttons.leftBumper;

							if (glm::length(this->inputState.analogStickLeft) > this->parameters.deadZone.get()) {
								args.movement1 = this->inputState.analogStickLeft * this->parameters.speed.get();
							}

							if (glm::length(this->inputState.analogStickRight) > this->parameters.deadZone.get()) {
								args.movement2 = this->inputState.analogStickRight * this->parameters.speed.get();
							}

							args.buttons[0] = this->inputState.buttons.cross && !this->priorInputState.buttons.cross;
							args.buttons[1] = this->inputState.buttons.circle && !this->priorInputState.buttons.circle;
							args.buttons[2] = this->inputState.buttons.triangle && !this->priorInputState.buttons.triangle;
							args.buttons[3] = this->inputState.buttons.square && !this->priorInputState.buttons.square;
						}

						// Check if needs to send
						auto defaultArgs = ofxRulr::RemoteControllerArgs();
						bool needsSend = memcmp(&args
							, &defaultArgs
							, sizeof(ofxRulr::RemoteControllerArgs));

						if (needsSend) {
							input->remoteControl(args);
						}
					}
				}
			}

			//----------
			void
				Controller::setDeviceIndex(size_t index)
			{
				auto controllers = ofxDualSense::Controller::listControllers();
				if (index < 0 || controllers.empty()) {
					this->controller.reset();
					this->parameters.index.set(-1);
				}
				else {
					// use the controllers that are available only
					index = min(index, controllers.size() - 1);
					this->controller = controllers[index];
					this->parameters.index.set(index);
				}
				this->refreshPanel();
			}

			//----------
			ofxCvGui::PanelPtr
				Controller::getPanel()
			{
				return this->panel;
			}

			//----------
			void
				Controller::refreshPanel()
			{
				this->panel->clear();

				if (!this->controller)
				{
					// Make a list of available controllers
					auto widgetsPanel = ofxCvGui::Panels::makeWidgets();
					auto controllers = ofxDualSense::Controller::listControllers();

					size_t index = 0;
					for (auto controller : controllers) {
						widgetsPanel->addButton(ofToString(index), [this, index]() {
							this->setDeviceIndex(index);
							})->addToolTip(controller->getDevicePath());
						index++;
					}
					widgetsPanel->addButton("Refresh", [this]() {
						this->refreshPanel();
						});
					this->panel->add(widgetsPanel);
				}
				else {
					// Show the current controller
					auto panel = ofxCvGui::Panels::makeBlank();
					panel->onDraw += [this](ofxCvGui::DrawArguments& args) {
						this->inputState.draw(args.localBounds);
					};
					this->panel->add(panel);
				}
			}
		}
	}
}
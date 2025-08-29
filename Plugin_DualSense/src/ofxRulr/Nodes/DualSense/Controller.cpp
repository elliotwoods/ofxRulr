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
				RULR_NODE_INSPECTOR_LISTENER;

				this->panel = ofxCvGui::Panels::Groups::makeStrip();
				this->refreshPanel();

				this->addInput<Nodes::Base>();

				this->manageParameters(this->parameters);
				this->onDeserialize += [this](const nlohmann::json&) {
					this->setDeviceIndex(this->parameters.index);
				};

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
					auto color = input
						? input->getColor()
						: ofColor(255, 0, 0);

					// Is active?
					{
						auto now = chrono::system_clock::now();

						// check if right trigger is pressed (this activates the controller for a while)
						if (this->inputState.rightTrigger > 0.2f) {
							this->lastActivated = now;
							if (!this->isActive) {
								this->lastActivatedBegin = now;
							}
						}

						auto timeSinceLastActivated = (float)chrono::duration_cast<chrono::milliseconds>(now - this->lastActivated).count() / 1000.0f;
						auto timeSinceLastActivatedBegin = (float) chrono::duration_cast<chrono::milliseconds>(now - this->lastActivatedBegin).count() / 1000.0f;

						this->isActive = timeSinceLastActivated <= this->parameters.activeTime.get();

						auto ratioWithinActiveTime = 1.0f - ofClamp(timeSinceLastActivated / this->parameters.activeTime.get(), 0, 1.0f);

						ofxDualSense::OutputState outputState;
						{
							outputState.softRumble = timeSinceLastActivatedBegin < 0.5f;
							
							outputState.playerLEDs.bitmask = DS5W_OSTATE_PLAYER_LED_LEFT & (ratioWithinActiveTime > 0)
								| DS5W_OSTATE_PLAYER_LED_MIDDLE_LEFT & (ratioWithinActiveTime > 0.2)
								| DS5W_OSTATE_PLAYER_LED_MIDDLE & (ratioWithinActiveTime > 0.4)
								| DS5W_OSTATE_PLAYER_LED_MIDDLE_RIGHT & (ratioWithinActiveTime > 0.6)
								| DS5W_OSTATE_PLAYER_LED_RIGHT & (ratioWithinActiveTime > 0.8);
							outputState.playerLEDs.brightness = DS5W::LedBrightness::MEDIUM;

							color.setBrightness(ratioWithinActiveTime * 255.0f);

							outputState.lightbar = this->isActive
								? color
								: ofColor(ofMap(sin(ofGetElapsedTimef()), -1, 1, 0, 100), 0, 0);
						}
						this->controller->setOutputState(outputState);
					}

					if (this->isActive) {
						if (input) {
							bool needsSend = false;

							// we have a target node
							ofxRulr::RemoteControllerArgs args;
							{
								needsSend |= args.next = this->inputState.DPad.x > 0 && !(this->priorInputState.DPad.x > 0)
									|| this->inputState.buttons.rightBumper && !this->priorInputState.buttons.rightBumper;

								needsSend |= args.previous = this->inputState.DPad.x < 0 && !(this->priorInputState.DPad.x < 0)
									|| this->inputState.buttons.leftBumper && !this->priorInputState.buttons.leftBumper;

								bool hasMovement = false;

								// Analog stick 1
								for (int i = 0; i < 2; i++) {
									if (abs(this->inputState.analogStickLeft[i]) > this->parameters.deadZone.get()) {
										args.analog1[i] = this->inputState.analogStickLeft[i];
										hasMovement = true;
									}
								}

								// Analog stick 2
								for (int i = 0; i < 2; i++) {
									if (abs(this->inputState.analogStickRight[i]) > this->parameters.deadZone.get()) {
										args.analog2[i] = this->inputState.analogStickRight[i];
										hasMovement = true;
									}
								}

								// Combined movement
								if (hasMovement) {
									args.combinedMovement = args.analog1 * this->parameters.coarseSpeed.get()
										+ args.analog2 * this->parameters.fineSpeed.get();
									needsSend = true;
								}

								// Digital stick
								args.digital = this->inputState.DPad;
								needsSend |= this->inputState.DPad != this->priorInputState.DPad;

								// Square, circle, etc
								needsSend |= args.buttons[0] = this->inputState.buttons.cross && !this->priorInputState.buttons.cross;
								needsSend |= args.buttons[1] = this->inputState.buttons.circle && !this->priorInputState.buttons.circle;
								needsSend |= args.buttons[2] = this->inputState.buttons.triangle && !this->priorInputState.buttons.triangle;
								needsSend |= args.buttons[3] = this->inputState.buttons.square && !this->priorInputState.buttons.square;
							}

							if (needsSend) {
								input->remoteControl(args);
							}
						}
					}
				}
			}

			//----------
			void
				Controller::populateInspector(ofxCvGui::InspectArguments& args)
			{
				auto inspector = args.inspector;
				inspector->addIndicatorBool("Active", [this]() {
					return this->isActive;
					});

				if (this->controller) {
					inspector->addButton("Close device", [this]() {
						this->closeDevice();
						});
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
			void
				Controller::closeDevice()
			{
				this->controller.reset();
				this->parameters.index.set(-1);
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
						auto activeStateRegionHeight = 30;

						// Draw active state
						{
							auto activeStateBounds = args.localBounds;
							activeStateBounds.height = activeStateRegionHeight;
							if (this->isActive) {
								ofPushStyle();
								{
									ofSetColor(100, 200, 100);
									ofDrawRectangle(activeStateBounds);
								}
								ofPopStyle();

								ofxCvGui::Utils::drawText("Active", activeStateBounds, false);
							}
							else {
								ofxCvGui::Utils::drawText("Press R2 to activate", activeStateBounds, false);
							}
						}
						// Draw input state
						{
							auto inputStateBounds = args.localBounds;
							{
								inputStateBounds.y += activeStateRegionHeight;
								inputStateBounds.height -= activeStateRegionHeight;
							}
							this->inputState.draw(inputStateBounds);
						}
					};
					this->panel->add(panel);
				}

				// Refresh inspector if we're selected
				ofxCvGui::refreshInspector(this);
			}
		}
	}
}
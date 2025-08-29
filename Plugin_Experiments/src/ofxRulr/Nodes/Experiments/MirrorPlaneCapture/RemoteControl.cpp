#include "pch_Plugin_Experiments.h"

namespace ofxRulr {
	namespace Nodes {
		namespace Experiments {
			namespace MirrorPlaneCapture {
				//---------
				RemoteControl::RemoteControl()
				{
					RULR_NODE_INIT_LISTENER;
				}

				//---------
				string
					RemoteControl::getTypeName() const
				{
					return "Halo::RemoteControl";
				}

				//---------
				void
					RemoteControl::init()
				{
					RULR_NODE_INSPECTOR_LISTENER;
					RULR_NODE_UPDATE_LISTENER;
					RULR_NODE_REMOTE_CONTROL_LISTENER;
					RULR_NODE_DRAW_WORLD_LISTENER;

					this->manageParameters(this->parameters);

					this->addInput<Heliostats2>();
				}

				//---------
				void
					RemoteControl::update()
				{

				}
				
				//---------
				void
					RemoteControl::drawWorldStage()
				{
					auto single = this->getSingle();
					if (single) {
						const auto & position = single->getHeliostatActionModelParameters().position;
						ofPushMatrix();
						{
							ofTranslate(position);
							// Draw diamond
							{
								// Bottom
								ofDrawLine({ 0, -0.3, 0 }, { 0.1, -0.5, 0 });
								ofDrawLine({ 0, -0.3, 0 }, { -0.1, -0.5, 0 });
								ofDrawLine({ 0, -0.3, 0 }, { 0, -0.5, 0.1 });
								ofDrawLine({ 0, -0.3, 0 }, { 0, -0.5, -0.1 });

								// Waist
								ofDrawLine({ -0.1, -0.5, 0 }, { 0, -0.5, 0.1 });
								ofDrawLine({ 0, -0.5, 0.1 }, { 0.1, -0.5, 0 });
								ofDrawLine({ 0.1, -0.5, 0 }, { 0, -0.5, -0.1 });
								ofDrawLine({ 0, -0.5, -0.1 }, { -0.1, -0.5, 0 });

								// Top
								ofDrawLine({ 0, -0.7, 0 }, { 0.1, -0.5, 0 });
								ofDrawLine({ 0, -0.7, 0 }, { -0.1, -0.5, 0 });
								ofDrawLine({ 0, -0.7, 0 }, { 0, -0.5, 0.1 });
								ofDrawLine({ 0, -0.7, 0 }, { 0, -0.5, -0.1 });
							}
						}
						ofPopMatrix();
					}
				}

				//---------
				void
					RemoteControl::populateInspector(ofxCvGui::InspectArguments& inspectArgs)
				{
					auto inspector = inspectArgs.inspector;

						//Key controls
					{
						auto element = make_shared<ofxCvGui::Element>();
						auto movementEnabled = make_shared<ofParameter<bool>>("Enabled", true);

						//we put it into the scope of the elemnt so that it will be deleted when that scope is deleted
						auto enabledButton = make_shared<ofxCvGui::Widgets::Toggle>(*movementEnabled);
						enabledButton->addListenersToParent(element);

						element->onDraw += [this, enabledButton, movementEnabled](ofxCvGui::DrawArguments& args) {
							auto drawKey = [movementEnabled](char key, float x, float y) {
								ofPushStyle();
								{
									auto keyIsPressed = movementEnabled->get() ? (ofGetKeyPressed(key) || ofGetKeyPressed(key - ('A' - 'a'))) : false;
									if (keyIsPressed) {
										ofFill();
									}
									else {
										ofSetLineWidth(1.0f);
										ofNoFill();
									}
									ofDrawRectangle(x, y, 30, 30);

									if (keyIsPressed) {
										ofSetColor(40);
									}
									else {
										ofSetColor(200);
									}
									ofDrawBitmapString(string(1, key), x + (30 - 8) / 2, y + (30 + 10) / 2);
								}
								ofPopStyle();
							};
							ofPushStyle();
							{
								ofDrawBitmapString("Selection", 10, 10);
								ofDrawBitmapString("Single", 10, 50 + 30 + 10 + 10);

								{
									ofSetColor(150, 200, 150);
									drawKey('W', 50, 10);
									drawKey('A', 10, 50);

									ofSetColor(200, 150, 150);
									drawKey('S', 50, 50);
									drawKey('D', 90, 50);
								}

								{

									ofSetColor(150, 200, 150);
									drawKey('8', 50, 50 + 30 + 10 + 10 + 10);
									drawKey('5', 50, 50 + 30 + 10 + 50 + 10);

									ofSetColor(200, 150, 150);
									drawKey('4', 10, 50 + 30 + 10 + 50 + 10);
									drawKey('6', 90, 50 + 30 + 10 + 50 + 10);
								}
							}
							ofPopStyle();
						};
						element->onKeyboard += [this, enabledButton, movementEnabled](ofxCvGui::KeyboardArguments& args) {
							auto speed = this->parameters.speedKeyboard.get();

							if (movementEnabled && args.action == ofxCvGui::KeyboardArguments::Action::Pressed) {
								int axes = -1;
								bool forSingle = false;

								switch (args.key) {
								case '8':
								case '5':
									forSingle = true;
								case 'w':
								case 'W':
								case 's':
								case 'S':
									axes = 1;
									break;

								case '4':
								case '6':
									forSingle = true;
								case 'a':
								case 'A':
								case 'd':
								case 'D':
									axes = 0;
									break;
								default:
									//no action
									return;
									break;
								};

								float difference = 1.0f;
								switch (args.key)
								{
								case 'w':
								case 'W':
								case 'a':
								case 'A':
								case '5':
								case '4':
									difference = -1.0f;
									break;

								case 's':
								case 'S':
								case 'd':
								case 'D':
								case '8':
								case '6':
									difference = +1.0f;
									break;

								default:
									break;
								}

								try {
									switch (axes) {
									case 0:
										if (forSingle) {
											this->moveSingle({ difference * speed, 0.0f });
										}
										else {
											this->moveSelection({ difference * speed, 0.0f });
										}
										break;
									case 1:
										if (forSingle) {
											this->moveSingle({ 0.0f, difference * speed });
										}
										else {
											this->moveSelection({ 0.0f, difference * speed });
										}
										break;
									default:
										break;
									}
								}
								RULR_CATCH_ALL_TO_ERROR;
							}
						};
						element->onBoundsChange += [enabledButton](ofxCvGui::BoundsChangeArguments& args) {
							auto bounds = args.localBounds;
							bounds.x = 150 + 30 + 10;
							bounds.width -= bounds.x;
							enabledButton->setBounds(bounds);
						};
						element->setHeight(80.0f + 60.0f + 50);
						inspector->add(element);
					}

					inspector->addButton("Home single", [this]() {
						try {
							this->homeSingle();
						}
						RULR_CATCH_ALL_TO_ALERT;
						}, '+');
					inspector->addButton("Home selection", [this]() {
						try {
							this->homeSelection();
						}
						RULR_CATCH_ALL_TO_ALERT;
						}, ' ');

					inspector->addButton("Flip single", [this]() {
						try {
							this->flipSingle();
						}
						RULR_CATCH_ALL_TO_ALERT;
						});
					inspector->addButton("Flip selection", [this]() {
						try {
							this->flipSelection();
						}
						RULR_CATCH_ALL_TO_ALERT;
						}, 'f');
				}
				//---------
				void
					RemoteControl::remoteControl(const RemoteControllerArgs& args)
				{
					auto speed = this->parameters.speedGameController.get() * ofGetElapsedTimef() / 100.0f;
					glm::vec2 directionality(1.0f, -1.0f);
					float power = 2.0f;

					auto getMovement = [&](glm::vec2 stick) {
						auto value = stick;
						value.x = pow(value.x, power) * (value.x < 0 ? -1 : 1);
						value.y = pow(value.y, power) * (value.y < 0 ? -1 : 1);
						value *= speed;
						value *= directionality;
						return value;
					};

					try {
						// Analog 1 = Move single
						if (glm::length(args.analog1) > 0) {
							auto movement = getMovement(args.analog1);
							this->moveSingle(movement);
						}

						// Analog 2 = Move selection
						if (glm::length(args.analog2) > 0) {
							auto movement = getMovement(args.analog2);
							this->moveSelection(movement);
						}

						// Button 0 = Home selection
						if (args.buttonDown.cross) {
							this->homeSelection();
						}

						// Button 1 = Home single
						if (args.buttonDown.circle) {
							this->homeSingle();
						}

						// Button 2 = Inspect single
						if (args.buttonDown.triangle) {
							auto single = this->getSingle();
							if (single) {
								ofxCvGui::inspect(single);
							}
						}

						// Change heliostat selection
						if(args.next || args.previous) {
							// Get Heliostats2
							this->throwIfMissingAConnection<Heliostats2>();
							auto heliostatsNode = this->getInput<Heliostats2>();

							// Gather heliostats from node
							vector<string> heliostatNames;
							auto selectedHeliostats = heliostatsNode->getHeliostats();
							for (auto heliostat : selectedHeliostats) {
								heliostatNames.push_back(heliostat->getName());
							}

							auto currentSelectionIt = std::find(heliostatNames.begin(), heliostatNames.end(), this->parameters.singleName.get());
							if (currentSelectionIt == heliostatNames.end()) {
								// Couldn't find current selection
								if (!heliostatNames.empty()) {
									// Just select the first one in the names list
									this->parameters.singleName.set(heliostatNames.front());
								}
								else {
									throw(ofxRulr::Exception("No heliostats to select"));
								}
							}
							else {
								// There are heliostats

								if (args.next) {
									// Go to next
									currentSelectionIt++;

									// If end then wrap around
									if (currentSelectionIt == heliostatNames.end()) {
										currentSelectionIt = heliostatNames.begin();
									}
								}
								else if (args.previous) {
									// If at start then wrap around
									if (currentSelectionIt == heliostatNames.begin()) {
										currentSelectionIt = heliostatNames.end();
									}

									// Go to previous
									currentSelectionIt--;
								}

								this->parameters.singleName.set(*currentSelectionIt);
							}
						}

					}
					RULR_CATCH_ALL_TO_ERROR;
				}

				//---------
				shared_ptr<Heliostats2::Heliostat>
					RemoteControl::getSingle()
				{
					shared_ptr<Heliostats2::Heliostat> single;
					auto heliostatsNode = this->getInput<Heliostats2>();
					if (heliostatsNode) {
						auto heliostats = heliostatsNode->getHeliostats();
						for (auto heliostat : heliostats) {
							if (heliostat->parameters.name.get() == this->parameters.singleName.get()) {
								return heliostat;
							}
						}
					}
					return single;
				}
					
				//---------
				void
					RemoteControl::moveSingle(const glm::vec2& movement)
				{
					auto single = this->getSingle();
					if (single) {
						this->move(single, movement);
					}
				}

				//---------
				void
					RemoteControl::moveSelection(const glm::vec2& movement)
				{
					auto heliostatsNode = this->getInput<Heliostats2>();
					if (heliostatsNode) {
						auto heliostats = heliostatsNode->getHeliostats();
						for (auto heliostat : heliostats) {
							this->move(heliostat, movement);
						}
					}
				}

				//---------
				void
					RemoteControl::move(shared_ptr<Heliostats2::Heliostat> heliostat, const glm::vec2& movement)
				{
					heliostat->parameters.servo1.angle.set(heliostat->parameters.servo1.angle.get() + movement.x);
					heliostat->parameters.servo2.angle.set(heliostat->parameters.servo2.angle.get() + movement.y);
				}

				//---------
				void
					RemoteControl::homeSingle()
				{
					auto single = this->getSingle();
					if (single) {
						this->home(single);
					}
				}
				//---------
				void
					RemoteControl::homeSelection()
				{
					this->throwIfMissingAnyConnection();
					auto heliostatsNode = this->getInput<Heliostats2>();
					auto heliostats = heliostatsNode->getHeliostats();
					for (auto heliostat : heliostats) {
						this->home(heliostat);
					}
				}

				//---------
				void
					RemoteControl::flipSingle()
				{
					this->throwIfMissingAnyConnection();
					auto heliostatsNode = this->getInput<Heliostats2>();
					auto heliostats = heliostatsNode->getHeliostats();
					for (auto heliostat : heliostats) {
						if (heliostat->parameters.name.get() == this->parameters.singleName.get()) {
							heliostat->flip();
						}
					}
				}

				//---------
				void
					RemoteControl::flipSelection()
				{
					this->throwIfMissingAnyConnection();
					auto heliostatsNode = this->getInput<Heliostats2>();
					auto heliostats = heliostatsNode->getHeliostats();
					Utils::ScopedProcess scopedProcess("Flip heliostats", heliostats.size());
					for (auto heliostat : heliostats) {
						try {
							Utils::ScopedProcess scopedProcessHeliostat(heliostat->getName());
							heliostat->flip();
							scopedProcessHeliostat.end();
						}
						RULR_CATCH_ALL_TO_ERROR;
					}
				}

				//---------
				void
					RemoteControl::home(shared_ptr<Heliostats2::Heliostat> heliostat)
				{
					heliostat->parameters.servo1.angle.set(0.0f);
					heliostat->parameters.servo2.angle.set(0.0f);
				}
			}
		}
	}
}
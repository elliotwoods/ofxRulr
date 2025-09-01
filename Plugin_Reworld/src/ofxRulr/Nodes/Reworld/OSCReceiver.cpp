#include "pch_Plugin_Reworld.h"
#include "OSCReceiver.h"

#include "Installation.h"

namespace ofxRulr {
	namespace Nodes {
		namespace Reworld {
			//------------
			OSCReceiver::OSCReceiver()
			{
				RULR_NODE_INIT_LISTENER;
			}

			//------------
			string
				OSCReceiver::getTypeName() const
			{
				return "Reworld::OSCReceiver";
			}

			//------------
			void
				OSCReceiver::init()
			{
				RULR_NODE_UPDATE_LISTENER;
				RULR_NODE_INSPECTOR_LISTENER;
				RULR_NODE_DRAW_WORLD_LISTENER;

				this->addInput<Installation>();
				this->addInput<Item::RigidBody>("Light");

				this->manageParameters(this->parameters);
			}

			//------------
			void
				OSCReceiver::update()
			{
				if (this->oscReceiver) {
					if (this->parameters.port.get() != this->oscReceiver->getPort()) {
						this->oscReceiver.reset();
					}

					if (!this->parameters.enabled) {
						this->oscReceiver.reset();
					}
				}

				if (!this->oscReceiver && this->parameters.enabled) {
					this->oscReceiver = make_unique<ofxOscReceiver>();
					this->oscReceiver->setup(this->parameters.port.get());
				}

				if (this->oscReceiver) {
					ofxOscMessage message;

					auto rxBegin = std::chrono::system_clock::now();
					auto rxMustEnd = rxBegin + std::chrono::milliseconds((long long)(this->parameters.maxRxTimePerFrame * 1000.0f));

					size_t count = 0;

					while (this->oscReceiver->getNextMessage(message)) {
						try {
							this->throwIfMissingAConnection<Installation>();
							auto installation = this->getInput<Installation>();

							this->oscFrameNew.notifyFrameNew = true;
							this->oscFrameNew.rxCount++;

							if (message.getAddress() == "/homeAndSeeThrough") {
								installation->homeAndZero();
								installation->gotoSeeThrogh();
							}
							else if (message.getAddress() == "/targetWorld") {
								// Format is:
								// [optional column offset integer], [float]*3, [float*3], [float*3]....
								// e.g.:
								// Data starts bottom left of installation and moves up first column then second column, etc
								// [float], [float], [float], etc...
								// 
								// Data starts at 8th column and moves up 8th column then goes to 9th column, etc
								// [int,e.g. 8], [float], [float], [float], etc...
								// 
								// Where the data goes into the modules starting at the first column from the bottom and then going onto the next column

								this->throwIfMissingAConnection<Item::RigidBody>();
								auto light = this->getInput<Item::RigidBody>();
								auto lightPosition = light->getPosition();

								auto previewEnabled = this->parameters.preview.enabled.get();

								auto argCount = message.getNumArgs();
								if (argCount < 1) {
									continue;
								}

								size_t argIndex = 0;
								size_t columnOffset = 0;

								// Column offset
								{
									if (message.getArgType(0) == ofxOscArgType::OFXOSC_TYPE_INT32
										|| message.getArgType(0) == ofxOscArgType::OFXOSC_TYPE_INT64)
									{
										columnOffset = message.getArgAsInt(argIndex++);
									}
								}

								auto columns = installation->getAllColumns();
								for (size_t columnIndex = columnOffset; columnIndex < columns.size(); columnIndex++) {
									auto column = columns[columnIndex];
									auto modules = column->getAllModules();
									for (size_t moduleIndex = 0; moduleIndex < modules.size(); moduleIndex++) {
										auto module = modules[moduleIndex];

										// continue until we run out of data
										if (argIndex + 3 <= argCount) {
											glm::vec3 target{
												message.getArgAsFloat(argIndex++)
												, message.getArgAsFloat(argIndex++)
												, message.getArgAsFloat(argIndex++)
											};

											auto result = Solvers::Reworld::Navigate::PointToPoint::solve(module->getModel()
												, module->getCurrentAxisAngles()
												, lightPosition
												, target
												, this->parameters.solverSettings.getSolverSettings());
											module->setTargetAxisAngles(result.solution.axisAngles);

											if (previewEnabled) {
												PreviewPoint previewPoint{
													target
													, module->getPosition()
													, module->color.get()
												};
												Router::Address address;
												{
													address.portal = moduleIndex;
													address.column = columnIndex;
												}
												this->previewData[address] = previewPoint;

												this->previewDataDirty = true;
											}
										}
									}
								}
							}
						}
						RULR_CATCH_ALL_TO_ERROR;

						count++;

						if (std::chrono::system_clock::now() > rxMustEnd) {
							RULR_ERROR << "Timeout receiving messages (" << ofToString(count) << " messages received)";

							// dump remaining messages
							ofxOscMessage message;
							while (this->oscReceiver->getNextMessage(message)) {};

							// exit out
							break;
						}
					}

				}

				{
					this->oscFrameNew.isFrameNew = this->oscFrameNew.notifyFrameNew;
					this->oscFrameNew.notifyFrameNew = false;
				}
				
				if (this->previewDataDirty) {
					this->updatePreviewData();
				}
			}

			//------------
			void
				OSCReceiver::drawWorldStage()
			{
				if (this->parameters.preview.enabled.get()) {
					this->preview.draw();
				}
			}

			//------------
			void
				OSCReceiver::updatePreviewData()
			{
				if (!this->parameters.preview.enabled.get()) {
					this->previewDataDirty = false;
					this->clearPreviewData();
					return;
				}

				this->preview.clear();

				auto r = this->parameters.preview.radius.get();
				auto colorEnabled = this->parameters.preview.colorsEnabled.get();

				this->preview.setMode(ofPrimitiveMode::OF_PRIMITIVE_LINES);

				glm::vec3 min({
					std::numeric_limits<float>::max()
					, std::numeric_limits<float>::max()
					, std::numeric_limits<float>::max()
					});
				glm::vec3 max({
					std::numeric_limits<float>::lowest()
					, std::numeric_limits<float>::lowest()
					, std::numeric_limits<float>::lowest()
					});

				for (const auto& it : this->previewData) {
					auto& target = it.second.position;
					auto& color = it.second.color;

					auto directionTowardsModule = glm::normalize(it.second.modulePosition - it.second.position);
					vector<glm::vec3> vertices{
						target - glm::vec3(r, 0, 0), target + glm::vec3(r, 0, 0)
						, target - glm::vec3(0, r, 0), target + glm::vec3(0, r, 0)
						, target - r * directionTowardsModule, target + r * directionTowardsModule
					};
					this->preview.addVertices(vertices);

					if (colorEnabled) {
						const auto color = (ofFloatColor)it.second.color;
						vector<ofFloatColor> colors{
							color, color
							, color, color
							, color, color
						};
						this->preview.addColors(colors);
					}

					for (int i = 0; i < 3; i++) {
						if (target[i] < min[i]) {
							min[i] = target[i];
						}
						if (target[i] > max[i]) {
							max[i] = target[i];
						}
					}
				}

				this->bounds.min = min;
				this->bounds.max = max;

				this->previewDataDirty = false;
			}

			//------------
			void
				OSCReceiver::clearPreviewData()
			{
				this->previewData.clear();
				this->preview.clear();
				this->bounds.max = { 0, 0, 0 };
				this->bounds.min = { 0, 0, 0 };
			}

			//------------
			void
				OSCReceiver::populateInspector(ofxCvGui::InspectArguments& args)
			{
				auto inspector = args.inspector;
				inspector->addTitle("OSC", ofxCvGui::Widgets::Title::Level::H2);
				inspector->addIndicatorBool("OSC Receiver active", [this]() {
					return (bool)this->oscReceiver;
					});
				inspector->addHeartbeat("OSC rx", [this]() {
					return this->oscFrameNew.isFrameNew;
					});
				inspector->addLiveValue<size_t>("Rx count", [this]() {
					return this->oscFrameNew.rxCount;
					});

				inspector->addTitle("Data", ofxCvGui::Widgets::Title::Level::H2);
				inspector->addLiveValue<size_t>("Point count (preview)", [this]() {
					return this->previewData.size();
					});

				inspector->addLiveValue<glm::vec3>("Bounds min", [this]() {
					return this->bounds.min;
					});
				inspector->addLiveValue<glm::vec3>("Bounds max", [this]() {
					return this->bounds.max;
					});
				inspector->addButton("Clear preview", [this]() {
					this->clearPreviewData();
					});
				inspector->addButton("Update preview", [this]() {
					this->updatePreviewData();
					});
			}
		}
	}
}

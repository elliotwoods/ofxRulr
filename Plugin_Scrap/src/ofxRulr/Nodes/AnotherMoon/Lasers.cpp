#include "pch_Plugin_Scrap.h"
#include "Lasers.h"


namespace ofxRulr {
	namespace Nodes {
		namespace AnotherMoon {
			//----------
			Lasers::Lasers()
			{
				RULR_NODE_INIT_LISTENER;
			}

			//----------
			string
				Lasers::getTypeName() const
			{
				return "AnotherMoon::Lasers";
			}

			//----------
			void
				Lasers::init()
			{
				RULR_NODE_UPDATE_LISTENER;
				RULR_NODE_SERIALIZATION_LISTENERS;
				RULR_NODE_DRAW_WORLD_LISTENER;
				RULR_NODE_INSPECTOR_LISTENER;

				this->manageParameters(this->parameters);

				// panel
				{
					this->panel = make_shared<ofxCvGui::Panels::Widgets>();
					this->lasers.populateWidgets(this->panel);
				}
			}

			//----------
			void
				Lasers::update()
			{
				{
					// laser has a RigidBody inside, so we should update it
					auto lasers = this->lasers.getAllCaptures();
					for (auto laser : lasers) {
						laser->update();
					}
				}

				if (this->parameters.signalEnabled.get()) {
					// Update state
					{
						if (this->parameters.pushState.scheduled) {
							auto timeSinceLastUpdate = chrono::system_clock::now() - this->lastPushStateTime;
							if (timeSinceLastUpdate > chrono::seconds(this->parameters.pushState.updatePeriod)) {
								this->pushState();
							}
						}
					}

					// Test image
					if (this->parameters.testImage.enabled) {
						this->sendTestImageToAll();
					}
				}
			}

			//----------
			void
				Lasers::drawWorldStage()
			{
				auto lasers = this->lasers.getSelection();
				for (auto laser : lasers) {
					laser->drawWorldStage();
				}
			}

			//----------
			void
				Lasers::serialize(nlohmann::json& json)
			{
				this->lasers.serialize(json["lasers"]);
			}

			//----------
			void
				Lasers::deserialize(const nlohmann::json& json)
			{
				if (json.contains("lasers")) {
					this->lasers.deserialize(json["lasers"]);
					
					auto allLasers = this->lasers.getAllCaptures();
					for (auto laser : allLasers) {
						laser->setParent(this);
					}
				}
			}

			//----------
			void
				Lasers::populateInspector(ofxCvGui::InspectArguments& inspectArgs)
			{
				auto inspector = inspectArgs.inspector;

				inspector->addButton("Import CSV", [this]() {
					try {
						this->importCSV();
					}
					RULR_CATCH_ALL_TO_ALERT;
					});

				inspector->addButton("Push state", [this]() {
					this->pushState();
					});

				inspector->addButton("Set size...", [this]() {
					auto result = ofSystemTextBoxDialog("Size");
					if (!result.empty()) {
						auto value = ofToFloat(result);
						this->setSize(value);
					}
					});

				inspector->addButton("Set brightness...", [this]() {
					auto result = ofSystemTextBoxDialog("Brightness");
					if (!result.empty()) {
						auto value = ofToFloat(result);
						this->setBrightness(value);
					}
					});
			}

			//----------
			ofxCvGui::PanelPtr
				Lasers::getPanel()
			{
				return this->panel;
			}

			//----------
			void
				Lasers::pushState()
			{
				auto allLasers = this->lasers.getAllCaptures();
				for (auto laser : allLasers) {
					// Ignore if we shouldn't send to deselected modules
					if (!laser->isSelected() && !this->parameters.sendToDeselected) {
						continue;
					}

					auto stateToSend = laser->isSelected()
						? this->parameters.selectedState.get()
						: this->parameters.deselectedState.get();

					switch (stateToSend.get()) {
					case Laser::State::Shutdown:
						laser->shutown();
						break;
					case Laser::State::Standby:
						laser->standby();
						break;
					case Laser::State::Run:
						laser->run();
						break;
					default:
						break;
					}
				}

				this->lastPushStateTime = chrono::system_clock::now();
			}

			//----------
			void
				Lasers::sendTestImageTo(const vector<shared_ptr<Laser>> lasers)
			{
				auto radius = this->parameters.testImage.circleRadius.get();

				float x = 0.0f;
				float y = 0.0f;

				if (this->parameters.testImage.animation.enabled.get()) {
					auto phase = ofGetElapsedTimef() / this->parameters.testImage.animation.period.get() * TWO_PI;
					auto movementRadius = this->parameters.testImage.animation.movementRadius.get();
					x = sin(phase) * movementRadius;
					y = cos(phase) * movementRadius;
				}

				for (auto laser : lasers) {
					laser->drawCircle({ x, y }, radius);
				}
			}

			//----------
			void
				Lasers::sendTestImageToAll()
			{
				auto lasers = this->lasers.getSelection();
				this->sendTestImageTo(lasers);
			}

			//----------
			void
				Lasers::setSize(float size)
			{
				auto lasers = this->lasers.getSelection();
				for (auto laser : lasers) {
					laser->setSize(size);
				}
			}

			//----------
			void
				Lasers::setBrightness(float brightness)
			{
				auto lasers = this->lasers.getSelection();
				for (auto laser : lasers) {
					laser->setBrightness(brightness);
				}
			}

			//----------
			void
				Lasers::setDefaultSettings()
			{
				auto lasers = this->lasers.getSelection();
				for (auto laser : lasers) {
					laser->setSize(1.0f);
					laser->setBrightness(1.0f);
					laser->setSource(Laser::Source::USB);
				}
			}

			//----------
			vector<shared_ptr<Laser>>
				Lasers::getSelectedLasers()
			{
				return this->lasers.getSelection();
			}

			//----------
			void
				Lasers::importCSV()
			{
				auto result = ofSystemLoadDialog("Select CSV file");
				if (result.bSuccess) {
					auto contents = (string) ofBuffer(ofFile(result.filePath));
					auto lines = ofSplitString(contents, "\n");
					for (const auto& line : lines) {
						auto columns = ofSplitString(line, ",");

						shared_ptr<Laser> laser;

						if (columns.size() >= 1) {
							auto laserAddress = ofToInt(columns[0]);

							// first check if we should be using an existing laser
							{
								auto priorLasers = this->lasers.getAllCaptures();
								bool foundInPriors = false;
								for (auto priorLaser : priorLasers) {
									if (priorLaser->parameters.settings.address.get() == laserAddress) {
										// It's in priors, use that
										laser = priorLaser;
										foundInPriors = true;
										break;
									}
								}

								if (!foundInPriors) {
									// It's not in priors, set this up as a new one
									laser = make_shared<Laser>();
									laser->setParent(this);
									laser->parameters.settings.address.set(laserAddress);
									this->lasers.add(laser);
								}
							}

						}

						if (columns.size() >= 4) {
							laser->getRigidBody()->setPosition({
								ofToFloat(columns[1])
								, ofToFloat(columns[2])
								, ofToFloat(columns[3])
								});
						}

						if (columns.size() >= 7) {
							laser->getRigidBody()->setRotationEuler({
								ofToFloat(columns[4])
								, ofToFloat(columns[5])
								, ofToFloat(columns[6])
								});
						}

						if (columns.size() >= 9) {
							laser->parameters.settings.fov.set({
								ofToFloat(columns[7])
								, ofToFloat(columns[8])
								});
						}
					}
				}
			}
		}
	}
}
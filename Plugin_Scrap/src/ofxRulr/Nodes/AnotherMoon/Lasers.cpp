#include "pch_Plugin_Scrap.h"
#include "Lasers.h"


namespace ofxRulr {
	namespace Nodes {
		namespace AnotherMoon {
#pragma mark Laser
			//----------
			Lasers::Laser::Laser()
			{
				RULR_SERIALIZE_LISTENERS;
			}

			//----------
			Lasers::Laser::~Laser()
			{
				this->shutown();
			}

			//----------
			void
				Lasers::Laser::setParent(Lasers* parent)
			{
				this->parent = parent;
			}

			//----------
			string
				Lasers::Laser::getDisplayString() const
			{
				stringstream ss;
				ss << this->parameters.settings.address << " : (" << this->parameters.settings.position << ") [" << this->parameters.settings.centerOffset << "]";
				return ss.str();
			}

			//----------
			void
				Lasers::Laser::serialize(nlohmann::json& json)
			{
				Utils::serialize(json, this->parameters);
			}

			//----------
			void
				Lasers::Laser::deserialize(const nlohmann::json& json)
			{
				Utils::deserialize(json, this->parameters);
			}

			//----------
			void
				Lasers::Laser::populateInspector(ofxCvGui::InspectArguments& inspectArgs)
			{
				
			}

			//----------
			void
				Lasers::Laser::drawWorldStage()
			{
				ofPushMatrix();
				{
					ofMultMatrix(this->getTransform());
					ofxCvGui::Utils::drawTextAnnotation(ofToString(this->parameters.settings.address), glm::vec3(0, 0, 0), this->color);
					ofDrawAxis(1.0f);
				}
				ofPopMatrix();
			}

			//----------
			glm::mat4
				Lasers::Laser::getTransform() const
			{
				return glm::translate(this->parameters.settings.position.get())
					* glm::mat4(glm::quat(this->parameters.settings.rotation.get()));
			}

			//----------
			void
				Lasers::Laser::shutown()
			{
				ofxOscMessage msg;
				msg.setAddress("/shutdown");
				this->sendMessage(msg);
			}

			//----------
			void
				Lasers::Laser::standby()
			{
				{
					ofxOscMessage msg;
					msg.setAddress("/standby");
					this->sendMessage(msg);
				}

				// Early LaserClient has spelling mistake - so we send this too
				{
					ofxOscMessage msg;
					msg.setAddress("/stanby");
					this->sendMessage(msg);
				}
			}

			//----------
			void
				Lasers::Laser::run()
			{
				ofxOscMessage msg;
				msg.setAddress("/run");
				this->sendMessage(msg);
			}

			//----------
			void
				Lasers::Laser::setBrightness(float value)
			{
				ofxOscMessage msg;
				msg.setAddress("/brightness");
				msg.addFloatArg(value);
				this->sendMessage(msg);
			}

			//----------
			void
				Lasers::Laser::setSize(float value)
			{
				ofxOscMessage msg;
				msg.setAddress("/size");
				msg.addFloatArg(value);
				this->sendMessage(msg);
			}

			//----------
			void
				Lasers::Laser::setSource(const Source& source)
			{
				int sourceIndex = 1;
				switch (source.get()) {
				case Source::Circle:
					sourceIndex = 0;
					break;
				case Source::USB:
					sourceIndex = 1;
					break;
				case Source::Memory:
					sourceIndex = 2;
					break;
				default:
					break;
				}
				
				ofxOscMessage msg;
				msg.setAddress("/source");
				msg.addInt32Arg(sourceIndex);
				this->sendMessage(msg);
			}

			//----------
			void
				Lasers::Laser::drawCircle(glm::vec2 center, float radius)
			{
				center += this->parameters.settings.centerOffset.get();

				ofxOscMessage msg;
				msg.setAddress("/circle");
				msg.addFloatArg(center.x);
				msg.addFloatArg(center.y);
				msg.addFloatArg(radius);
				this->sendMessage(msg);
			}

			//----------
			string
				Lasers::Laser::getHostname() const
			{
				return this->parent->parameters.baseAddress.get() + ofToString(this->parameters.settings.address);
			}

			//----------
			void
				Lasers::Laser::sendMessage(const ofxOscMessage & msg)
			{
				if (this->oscSender) {
					if (this->oscSender->getHost() != this->getHostname()
						|| this->oscSender->getPort() != this->parent->parameters.remotePort.get()) {
						this->oscSender.reset();
					}
				}
				
				if (!this->oscSender) {
					this->oscSender = make_unique<ofxOscSender>();
					this->oscSender->setup(this->getHostname(), this->parent->parameters.remotePort.get());
				}

				// can put some error checking here in-between

				if (this->oscSender) {
					this->oscSender->sendMessage(msg);
				}
			}

#pragma mark Lasers
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
					case State::Shutdown:
						laser->shutown();
						break;
					case State::Standby:
						laser->standby();
						break;
					case State::Run:
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
					laser->setSource(Source::USB);
				}
			}

			//----------
			vector<shared_ptr<Lasers::Laser>>
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

						shared_ptr<Lasers::Laser> laser;

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
									laser = make_shared<Lasers::Laser>();
									laser->setParent(this);
									laser->parameters.settings.address.set(laserAddress);
									this->lasers.add(laser);
								}
							}

						}

						if (columns.size() >= 4) {
							laser->parameters.settings.position.set({
								ofToFloat(columns[1])
								, ofToFloat(columns[2])
								, ofToFloat(columns[3])
								});
						}

						if (columns.size() >= 7) {
							laser->parameters.settings.rotation.set({
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
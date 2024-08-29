#include "pch_Plugin_Scrap.h"
#include "Lasers.h"

using namespace ofxRulr::Data::AnotherMoon;

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
				RULR_NODE_DRAW_WORLD_ADVANCED_LISTENER;
				RULR_NODE_INSPECTOR_LISTENER;

				this->manageParameters(this->parameters);

				// panel
				{
					this->panel = make_shared<ofxCvGui::Panels::Widgets>();
					this->lasers.populateWidgets(this->panel);
				}

				this->messageRouter.init(MessageRouter::Port::Server
					, MessageRouter::Port::Client);
			}

			//----------
			void
				Lasers::update()
			{
				// Update all lasers
				{
					auto lasers = this->lasers.getAllCaptures();
					for (auto laser : lasers) {
						laser->update();
					}
				}

				// Set state by selection
				if (this->parameters.setStateBySelected.enabled.get()) {
					this->setStateBySelection();
				}

				// Scheduled full send
				{
					if (this->parameters.pushFullState.enabled) {
						auto duration = chrono::system_clock::now() - this->lastPushAllTime;
						if (duration > chrono::seconds(this->parameters.pushFullState.updatePeriod)) {
							this->pushAllSelected();
						}
					}
				}

				// Test image
				if (this->parameters.testImage.enabled) {
					this->sendTestImageToSelected();
				}

				// Perform communications
				{
					auto lasers = this->getLasersAll();

					// Messages
					{
						shared_ptr<IncomingMessage> incomingMessage;
						while (this->messageRouter.getIncomingMessage(incomingMessage)) {
							// find the matching laser
							for (auto laser : lasers) {
								if (laser->getHostname() == incomingMessage->getRemoteHost()) {
									laser->processIncomingMessage(incomingMessage);
									break;
								}
							}
						}
					}

					// ACKs
					{
						shared_ptr<AckMessageIncoming> incomingMessage;
						while (this->messageRouter.getIncomingAck(incomingMessage)) {
							// find the matching laser
							for (auto laser : lasers) {
								if (laser->getHostname() == incomingMessage->getRemoteHost()) {
									laser->processIncomingAck(incomingMessage);
									break;
								}
							}
						}

						if (this->parameters.communications.logAcks.get()) {
							this->messageRouter.setAckTimeLoggingEnabled(true);

							int ackTime;
							vector<int> ackTimes;
							while (this->messageRouter.ackTime.tryReceive(ackTime)) {
								ackTimes.push_back(ackTime);
							}
							if (!ackTimes.empty()) {
								static auto file = std::fopen(ofToDataPath("logAcks.txt").c_str(), "w");
								char message[100];
								for (const auto& ackTime : ackTimes) {
									sprintf(message, "%d\n", ackTime);
									std::fputs(message, file);
								}
							}
						}
						else {
							this->messageRouter.setAckTimeLoggingEnabled(false);
						}
					}
				}
			}

			//----------
			void
				Lasers::drawWorldAdvanced(DrawWorldAdvancedArgs& drawArgs)
			{
				Laser::DrawArguments args{ drawArgs };
				{
					args.rigidBody = ofxRulr::isActive(this, this->parameters.draw.rigidBody);
					args.trussLine = ofxRulr::isActive(this, this->parameters.draw.trussLine);
					args.centerRay = ofxRulr::isActive(this, this->parameters.draw.centerRay);
					args.modelPreview = ofxRulr::isActive(this, this->parameters.draw.modelPreview);
					args.picture = ofxRulr::isActive(this, this->parameters.draw.picturePreview);
					args.pictureBrightness = this->parameters.draw.pictureBrightness.get();

					// Ground height
					{
						auto worldPanelUntyped = ofxRulr::Graph::World::X().getWorldStage()->getPanel();
						auto worldPanel = static_pointer_cast<ofxCvGui::Panels::WorldManaged>(worldPanelUntyped);
						args.groundHeight = worldPanel->parameters.grid.roomMax.get().y;
					}
				}

				auto lasers = this->lasers.getSelection();
				for (auto laser : lasers) {
					args.frustum = ofxRulr::isActive(laser.get(), this->parameters.draw.frustum);
					laser->drawWorldStage(args);
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

				inspector->addButton("Import JSON", [this]() {
					try {
						this->importJson();
					}
					RULR_CATCH_ALL_TO_ALERT;
					})->addToolTip("RunDeck make_nodes.py");

				inspector->addButton("Import CSV", [this]() {
					try {
						this->importCSV();
					}
					RULR_CATCH_ALL_TO_ALERT;
					})->addToolTip("Position, Serial, IP, x, y, z");

				inspector->addButton("Set state by selection", [this]() {
					this->setStateBySelection();
					});

				inspector->addButton("Push full state", [this]() {
					pushAllSelected();
					}, OF_KEY_RETURN)->setHeight(100.0f);
			}

			//----------
			ofxCvGui::PanelPtr
				Lasers::getPanel()
			{
				return this->panel;
			}

			//----------
			void
				Lasers::setStateBySelection()
			{
				auto allLasers = this->lasers.getAllCaptures();
				for (auto laser : allLasers) {
					// Ignore if we shouldn't send to deselected modules
					auto stateToSend = laser->isSelected()
						? this->parameters.setStateBySelected.selectedState.get()
						: this->parameters.setStateBySelected.deselectedState.get();

					laser->parameters.deviceState.state.set(stateToSend);
				}
			}

			//----------
			void
				Lasers::pushAllSelected()
			{
				auto lasers = this->getLasersSelected();
				for (auto laser : lasers) {
					laser->pushAll();
				}

				this->lastPushAllTime = chrono::system_clock::now();
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
				Lasers::sendTestImageToSelected()
			{
				auto lasers = this->lasers.getSelection();
				this->sendTestImageTo(lasers);
			}

			//----------
			vector<shared_ptr<Laser>>
				Lasers::getLasersSelected()
			{
				return this->lasers.getSelection();
			}

			//----------
			vector<shared_ptr<Laser>>
				Lasers::getLasersAll ()
			{
				return this->lasers.getAllCaptures();
			}

			//----------
			shared_ptr<Laser>
				Lasers::findLaser(int serialNumber, bool onlySelected)
			{
				auto lasers = onlySelected ? this->lasers.getSelection() : this->lasers.getAllCaptures();
				for (auto laser : lasers) {
					if (laser->parameters.serialNumber.get() == serialNumber) {
						return laser;
					}
				}

				// return null if not found
				return shared_ptr<Laser>();
			}


			//----------
			bool
				Lasers::sendMessage(shared_ptr<Data::AnotherMoon::OutgoingMessage> message)
			{
				if (this->isCommunicationEnabled()) {
					this->messageRouter.sendOutgoingMessage(message);
					return true;
				}
				else {
					message->onSent.set_exception(std::make_exception_ptr(Exception("Communications disabled")));
					return false;
				}
			}

			//----------
			bool
				Lasers::isCommunicationEnabled() const
			{
				return this->parameters.communications.enabled.get();
			}

			//----------
			void
				Lasers::shutdownAll()
			{
				auto lasers = this->getLasersAll();
				for (auto laser : lasers) {
					laser->shutdown();
				}
				this->parameters.setStateBySelected.selectedState.set(Laser::State::Shutdown);
				this->parameters.setStateBySelected.deselectedState.set(Laser::State::Shutdown);
			}

			//----------
			void
				Lasers::importJson()
			{
				// Load json (from our RunDeck format created by make_nodes.py)

				auto result = ofSystemLoadDialog("Select Json file");
				if (result.bSuccess) {
					auto priorLasers = this->lasers.getAllCaptures();
					set<size_t> previousPositionSelections;
					for (auto priorLaser : priorLasers) {
						if (priorLaser->isSelected()) {
							previousPositionSelections.insert(priorLaser->parameters.positionIndex.get());
						}
					}
					this->lasers.clear();

					auto fileContents = ofFile(result.filePath).readToBuffer();
					auto json = nlohmann::json::parse(fileContents);

					for (auto jsonModule : json) {
						shared_ptr<Laser> laser;

						// Do some checks
						{
							if (!jsonModule.contains("data")) {
								continue;
							}
						}

						auto serialNumber = jsonModule["data"]["serialNumber"].get<int>();
						auto positionIndex = jsonModule["data"]["positionIndex"].get<int>();
						auto hostname = jsonModule["hostname"].get<string>();

						// check if we should be using an existing laser
						{
							bool foundInPriors = false;
							for (auto priorLaser : priorLasers) {
								if (priorLaser->parameters.positionIndex.get() == positionIndex) {
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
								laser->parameters.positionIndex.set(positionIndex);
							}
						}

						// we always remove to start with, even if it's a prior
						this->lasers.add(laser);
						if (previousPositionSelections.find(laser->parameters.positionIndex.get()) != previousPositionSelections.end()) {
							laser->setSelected(true);
						}

						laser->parameters.serialNumber.set(serialNumber);
						laser->parameters.communications.hostname.set(hostname);
						
						{
							glm::vec3 worldPosition;
							worldPosition.x = jsonModule["data"]["worldPosition"]["x"];
							worldPosition.y = jsonModule["data"]["worldPosition"]["y"];
							worldPosition.z = jsonModule["data"]["worldPosition"]["z"];
							laser->getRigidBody()->setPosition(worldPosition);
						}
					}
				}
			}

			//----------
			void
				Lasers::importCSV()
			{
				auto result = ofSystemLoadDialog("Select CSV file");
				if (result.bSuccess) {
					// Load the file into a string
					auto file = ofFile(result.filePath, ofFile::Mode::ReadOnly, false);
					if (file.is_open()) {
						auto priorLasers = this->lasers.getAllCaptures();

						// Get previous selection
						set<size_t> previousPositionSelections;
						{
							for (auto priorLaser : priorLasers) {
								if (priorLaser->isSelected()) {
									previousPositionSelections.insert(priorLaser->parameters.positionIndex.get());
								}
							}
						}
						this->lasers.clear();

						// Read the file
						auto buffer = file.readToBuffer();

						// Iterate over lines
						for (auto line : buffer.getLines()) {
							// Get the values for one line
							auto values = ofSplitString(line, ",");
							if (values.size() >= 6) {
								// It's a valid line
								auto positionIndex = ofToInt(values[0]);
								auto serialNumber = ofToInt(values[1]);
								auto hostname = values[2];

								shared_ptr<Laser> laser;

								// check if we should be using an existing laser
								{
									bool foundInPriors = false;
									for (auto priorLaser : priorLasers) {
										if (priorLaser->parameters.positionIndex.get() == positionIndex) {
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
										laser->parameters.positionIndex.set(positionIndex);
									}
								}

								// we always remove to start with, even if it's a prior
								this->lasers.add(laser);
								if (previousPositionSelections.find(laser->parameters.positionIndex.get()) != previousPositionSelections.end()) {
									laser->setSelected(true);
								}

								laser->parameters.serialNumber.set(serialNumber);
								laser->parameters.communications.hostname.set(hostname);

								{
									glm::vec3 worldPosition;
									worldPosition.x = ofToFloat(values[3]);
									worldPosition.y = ofToFloat(values[4]);
									worldPosition.z = ofToFloat(values[5]);
									laser->getRigidBody()->setPosition(worldPosition);
								}

								// Incline angle
								if (values.size() >= 7) {
									auto rotationVector = laser->getRigidBody()->getRotationEuler();
									cout << rotationVector << endl;
									rotationVector.x = ofToFloat(values[6]) * DEG_TO_RAD;
									laser->getRigidBody()->setRotationEuler(rotationVector);
								}

							}
						}
					}
					else {
						throw(ofxRulr::Exception("Failed to open CSV"));
					}
				}
			}
		}
	}
}
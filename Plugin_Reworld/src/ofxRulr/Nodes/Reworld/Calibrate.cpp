#include "pch_Plugin_Reworld.h"
#include "Calibrate.h"
#include "Installation.h"
#include "CalibrateController.h"

namespace ofxRulr {
	namespace Nodes {
		namespace Reworld {
			//----------
			Calibrate::CalibrateControllerSession::CalibrateControllerSession()
			{
				static int controllerIndex = 0;
				ofColor color(255, 150, 150);
				color.setHueAngle(40 * controllerIndex++);
				this->color = color;
			}

			//----------
			Data::Reworld::ColumnIndex
				Calibrate::CalibrateControllerSession::getColumnIndex() const
			{
				return this->columnIndex;
			}

			//----------
			Data::Reworld::ModuleIndex
				Calibrate::CalibrateControllerSession::getModuleIndex() const
			{
				return this->moduleIndex;
			}

			//----------
			void
				Calibrate::CalibrateControllerSession::setColumnIndex(Data::Reworld::ColumnIndex columnIndex)
			{
				this->columnIndex = columnIndex;
				this->onSelectedModuleChange.notifyListeners();
			}

			//----------
			void
				Calibrate::CalibrateControllerSession::setModuleIndex(Data::Reworld::ModuleIndex moduleIndex)
			{
				this->moduleIndex = moduleIndex;
				this->onSelectedModuleChange.notifyListeners();
			}

			//----------
			ofColor
				Calibrate::CalibrateControllerSession::getColor() const
			{
				return this->color;
			}

			//----------
			Calibrate::Calibrate()
			{
				RULR_NODE_INIT_LISTENER;
			}

			//----------
			string
				Calibrate::getTypeName() const
			{
				return "Reworld::Calibrate";
			}

			//----------
			void
				Calibrate::init()
			{
				RULR_NODE_UPDATE_LISTENER;
				RULR_NODE_INSPECTOR_LISTENER;
				RULR_NODE_DRAW_WORLD_LISTENER;
				RULR_NODE_SERIALIZATION_LISTENERS;

				this->addInput<Installation>();
				this->addInput<Item::RigidBody>("Light position");

				{
					this->panel = ofxCvGui::Panels::makeWidgets();
					this->captures.populateWidgets(this->panel);
					this->panel->addButton("New capture", [this]() {
						this->newCapture();
						});
				}

				{
					this->parameters.initialisation.solverSettings.printEachStep.set(false);
					this->parameters.initialisation.solverSettings.printReport.set(false);
				}

				this->ourSelection.onSelectionChanged += [this]() {
					this->needsLoadCaptureData = true;
					this->needsCalculateParking = true;
					};

				this->manageParameters(this->parameters);
			}

			//----------
			void
				Calibrate::update()
			{
				if (this->needsLoadCaptureData) {
					if (this->ourSelection.selection) {
						try {
							this->moveToCapturePositions(this->ourSelection.selection);
						}
						RULR_CATCH_ALL_TO_ERROR;
					}
					this->needsLoadCaptureData = false;
				}

				if (this->calibrateControllerSessionDataChanged) {
					// Create a cached list of sessions
					{
						// we create a cached list because we sometimes need to check this many times per frame (e.g. drawing the capture panel)
						this->calibrateControllerSelections.clear();
						for (auto it : this->calibrateControllerSessions) {
							Router::Address address;
							{
								address.column = it.second->getColumnIndex();
								address.portal = it.second->getModuleIndex();
							}
							this->calibrateControllerSelections.emplace(address);
						}
					}

					this->needsCalculateParking = true;
					this->calibrateControllerSessionDataChanged = false;
				}

				// Parking
				if (this->needsCalculateParking) {
					this->calculateParking();
				}
			}

			//----------
			void
				Calibrate::drawWorldStage()
			{
				// Draw selections
				{
					auto installation = this->getInput<Installation>();
					if (!installation) return;

					for (auto it : this->calibrateControllerSessions) {
						auto name = it.first->getName();
						try {
							auto module = installation->getModuleByIndices(it.second->getColumnIndex(), it.second->getModuleIndex(), false);
							ofxCvGui::Utils::drawTextAnnotation(name, module->getPosition(), it.second->getColor());
						}
						catch (...) {

						}
					}
				}

				// Draw captures
				{
					auto captures = this->captures.getSelection();
					for (auto capture : captures) {
						capture->drawWorldStage();
					}
				}

			}

			//----------
			ofxCvGui::PanelPtr
				Calibrate::getPanel()
			{
				return this->panel;
			}


			//----------
			void
				Calibrate::serialize(nlohmann::json& json)
			{
				this->captures.serialize(json["captures"]);
			}

			//----------
			void
				Calibrate::deserialize(const nlohmann::json& json)
			{
				if (json.contains("captures")) {
					this->captures.deserialize(json["captures"]);
				}
				this->initCaptures();
			}

			//----------
			void
				Calibrate::populateInspector(ofxCvGui::InspectArguments args)
			{
				auto inspector = args.inspector;
				inspector->addButton("Calculate parking", [this]() {
					this->calculateParking();
					});
			}

			//----------
			shared_ptr<Calibrate::CalibrateControllerSession>
				Calibrate::registerCalibrateController(CalibrateController* calibrateController)
			{
				// just in case
				this->unregisterCalibrateController(calibrateController);

				auto calibrateControllerSession = make_shared<CalibrateControllerSession>();
				calibrateControllerSession->onSelectedModuleChange += [this]() {
					this->calibrateControllerSessionDataChanged = true;
					};
				this->calibrateControllerSessions.emplace(calibrateController, calibrateControllerSession);

				return calibrateControllerSession;
			}

			//----------
			void
				Calibrate::unregisterCalibrateController(CalibrateController* calibrateController)
			{
				auto it = this->calibrateControllerSessions.find(calibrateController);

				if (it != this->calibrateControllerSessions.end()) {
					this->calibrateControllerSessions.erase(it);
				}
			}

			//----------
			const set<Router::Address>& 
				Calibrate::getCalibrateControllerSelections() const
			{
				return this->calibrateControllerSelections;
			}


			//----------
			void
				Calibrate::initCaptures()
			{
				// Sub selections
				{
					auto captures = this->captures.getAllCaptures();
					for (auto capture : captures) {
						this->initCapture(capture);
					}
				}
			}

			//----------
			void
				Calibrate::initCapture(shared_ptr<Data::Reworld::Capture> capture)
			{
				capture->parentSelection = &this->ourSelection;
				capture->setParent(this);
				capture->init();
				
				capture->onModuleDataPointsChange.clear();
				capture->onModuleDataPointsChange += [this]() {
					this->needsCalculateParking = true;
					};
			}

			//----------
			shared_ptr<Data::Reworld::Capture>
				Calibrate::newCapture()
			{
				auto capture = make_shared<Data::Reworld::Capture>();
				this->initCapture(capture);
				this->captures.add(capture);
				return capture;
			}

			//----------
			void
				Calibrate::estimateModuleData(Data::Reworld::Capture * capture)
			{
				this->throwIfMissingAnyConnection();
				auto installation = this->getInput<Installation>();

				auto light = this->getInput<Item::RigidBody>();
				auto lightPosition = light->getPosition();

				if (!capture->getTargetIsSet()) {
					throw(Exception("No target is set for this capture yet"));
				}

				auto columns = installation->getAllColumns();
				for (int i = 0; i < columns.size(); i++) {
					const auto & column = columns[i];
					const auto modules = column->getAllModules();
					for (int j = 0; j < modules.size(); j++) {
						auto module = modules[j];
						auto result = Solvers::Reworld::Navigate::PointToPoint::solve(module->getModel()
							, module->getCurrentAxisAngles()
							, lightPosition
							, capture->getTarget());
						capture->initialiseModuleDataWithEstimate(i, j, result.solution.axisAngles);
					}
				}

				this->moveToCapturePositions(capture);
			}

			//----------
			void
				Calibrate::moveToCapturePositions(Data::Reworld::Capture* capture)
			{
				this->throwIfMissingAConnection<Installation>();
				auto installation = this->getInput<Installation>();

				for (const auto& columnIt : capture->moduleDataPoints) {
					for (const auto& moduleIt : columnIt.second) {
						auto module = installation->getModuleByIndices(columnIt.first, moduleIt.first, false);
						if (moduleIt.second->state != Data::Reworld::Capture::ModuleDataPoint::Unset) {
							module->setTargetAxisAngles(moduleIt.second->axisAngles);
						}
					}
				}
			}

			//----------
			void
				Calibrate::moveDataPoint(shared_ptr<CalibrateControllerSession> calibrateControllerSession, const glm::vec2& movement)
			{
				this->throwIfMissingAConnection<Installation>();
				auto installation = this->getInput<Installation>();

				auto module = installation->getModuleByIndices(calibrateControllerSession->getColumnIndex(), calibrateControllerSession->getModuleIndex(), true);
				if (!module) {
					throw(Exception("moveDataPoint - module is not available (e.g. not selected)"));
				}
				auto currentAxisAngles = module->getCurrentAxisAngles();
				auto currentPolar = Models::Reworld::axisAnglesToPolar(currentAxisAngles);
				auto currentVector = Models::Reworld::polarToVector(currentPolar);

				auto newVector = currentVector + movement;

				// Clamp to max length of 1
				auto newVectorLength = glm::length(newVector);
				if (newVectorLength > 1) {
					newVector /= newVectorLength;
				}

				auto newPolar = Models::Reworld::vectorToPolar(newVector);
				auto newAxisAngles = Models::Reworld::polarToAxisAngles(newPolar);

				// Send it to the module
				module->setTargetAxisAngles(newAxisAngles);
				
				// Save it to the current capture
				auto capture = ourSelection.selection;
				if (capture) {
					capture->setManualModuleData(calibrateControllerSession->getColumnIndex(), calibrateControllerSession->getModuleIndex(), newAxisAngles);
				}
			}

			//----------
			void
				Calibrate::markDataPointGood(shared_ptr<CalibrateControllerSession> calibrateControllerSession)
			{
				this->throwIfMissingAConnection<Installation>();
				auto installation = this->getInput<Installation>();

				auto module = installation->getModuleByIndices(calibrateControllerSession->getColumnIndex(), calibrateControllerSession->getModuleIndex(), true);
				if (!module) {
					throw(Exception("moveDataPoint - module is not available (e.g. not selected)"));
				}

				// Save it to the current capture
				auto capture = ourSelection.selection;
				if (!capture) {
					throw(Exception("No capture selected"));
				}
				capture->markDataPointGood(calibrateControllerSession->getColumnIndex(), calibrateControllerSession->getModuleIndex());
			}

			//----------
			void
				Calibrate::calculateParking()
			{
				auto selectedCapture = this->ourSelection.selection;
				if (!selectedCapture) {
					return;
				}

				auto installation = this->getInput<Installation>();
				if (!installation) {
					return;
				}

				bool parkingEnabled = this->parameters.control.parkNonSelected.enabled.get();

				// Perform parking
				for (const auto& columnDataPointsIt : selectedCapture->moduleDataPoints) {
					for (const auto& moduleDataPointIt : columnDataPointsIt.second) {
						auto axisAngles = moduleDataPointIt.second->axisAngles;
						auto module = installation->getModuleByIndices(columnDataPointsIt.first, moduleDataPointIt.first, false);
						auto& moduleDataPoint = moduleDataPointIt.second;

						if (moduleDataPoint->state == Data::Reworld::Capture::ModuleDataPoint::Unset) {
							// Don't do anything if there's no data at all
							continue;
						}


						Router::Address address;
						{
							address.portal = moduleDataPointIt.first;
							address.column = columnDataPointsIt.first;
						}

						auto isSelectedByAController = this->calibrateControllerSelections.find(address) != this->calibrateControllerSelections.end();

						if (parkingEnabled && !isSelectedByAController) {
							// calculate an offset for the position (a parked position)
							float offset = 0.0f;
							if (!module->isSelected()) {
								// unselected
								offset = this->parameters.control.parkNonSelected.parkingSpots.unselected.get();
							}
							else if (moduleDataPoint->state == Data::Reworld::Capture::ModuleDataPoint::Estimated) {
								offset = this->parameters.control.parkNonSelected.parkingSpots.estimated.get();
							}
							else if (moduleDataPoint->state == Data::Reworld::Capture::ModuleDataPoint::Set) {
								offset = this->parameters.control.parkNonSelected.parkingSpots.set.get();
							}
							else if (moduleDataPoint->state == Data::Reworld::Capture::ModuleDataPoint::Good) {
								offset = this->parameters.control.parkNonSelected.parkingSpots.good.get();
							}

							axisAngles.A += offset;
							axisAngles.B += offset;
						}

						module->setTargetAxisAngles(axisAngles);
					}
				}

				needsCalculateParking = false;
			}
		}
	}
}
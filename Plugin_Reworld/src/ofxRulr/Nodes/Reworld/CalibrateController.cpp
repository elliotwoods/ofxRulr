#include "pch_Plugin_Reworld.h"
#include "CalibrateController.h"
#include "Installation.h"

namespace ofxRulr {
	namespace Nodes {
		namespace Reworld {
			//----------
			CalibrateController::CalibrateController()
			{
				RULR_NODE_INIT_LISTENER;
			}

			//----------
			string
				CalibrateController::getTypeName() const
			{
				return "Reworld::CalibrateController";
			}

			//----------
			void
				CalibrateController::init()
			{
				RULR_NODE_UPDATE_LISTENER;
				RULR_NODE_REMOTE_CONTROL_LISTENER;
				RULR_NODE_INSPECTOR_LISTENER;

				{
					auto input = this->addInput<Calibrate>();
					input->onNewConnection += [this](shared_ptr<Calibrate> calibrateNode) {
						this->calibrateControllerSession = calibrateNode->registerCalibrateController(this);
						this->rebuildPanel();
						};
					input->onDeleteConnection += [this](shared_ptr<Calibrate> calibrateNode) {
						calibrateNode->unregisterCalibrateController(this);
						this->calibrateControllerSession.reset();
						this->rebuildPanel();
						};
				}

				{
					this->panel = make_shared<ofxCvGui::Panels::Widgets>();
					this->rebuildPanel();
				}

				this->manageParameters(this->parameters);
			}

			//----------
			void
				CalibrateController::update()
			{
				auto calibrateControllerSession = this->calibrateControllerSession.lock();
				if (calibrateControllerSession) {
					calibrateControllerSession->setName(this->getName());
				}
			}

			//----------
			void
				CalibrateController::populateInspector(ofxCvGui::InspectArguments args)
			{
				auto inspector = args.inspector;

				inspector->addButton("Select by mouse", [this]() {
					try {
						this->selectClosestModuleToMouseCursor();
					}
					RULR_CATCH_ALL_TO_ALERT;
					}, ' ');
			}

			//----------
			void
				CalibrateController::remoteControl(RemoteControllerArgs remoteControlArgs)
			{
				auto controllerSession = this->calibrateControllerSession.lock();
				if (!controllerSession) {
					return;
				}


				try {
					if (remoteControlArgs.buttonDown.up) {
						this->up();
					}
					if (remoteControlArgs.buttonDown.down) {
						this->down();
					}
					if (remoteControlArgs.buttonDown.left) {
						this->left();
					}
					if (remoteControlArgs.buttonDown.right) {
						this->right();
					}

					if (remoteControlArgs.buttonDown.cross) {
						this->x();
					}
					if (remoteControlArgs.buttonDown.circle) {
						this->circle();
					}
					if (remoteControlArgs.buttonDown.square) {
						this->square();
					}

					{
						auto movementMagnitude = glm::length(remoteControlArgs.analog2);

						if (movementMagnitude > 0) {
							auto movement = remoteControlArgs.analog2 * pow(movementMagnitude, this->parameters.movementPower.get());
							movement *= ofGetLastFrameTime() * this->parameters.movementSpeed.get();
							this->moveAxes(movement);
						}
					}
				}
				RULR_CATCH_ALL_TO_ERROR;
			}

			//----------
			ofxCvGui::PanelPtr
				CalibrateController::getPanel()
			{
				return this->panel;
			}

			//----------
			void
				CalibrateController::performAction(function<void(shared_ptr<Calibrate>, shared_ptr<Calibrate::CalibrateControllerSession>)> action)
			{
				auto calibrateControllerSession = this->calibrateControllerSession.lock();
				if (!calibrateControllerSession) {
					return;
				}

				this->throwIfMissingAConnection<Calibrate>();
				auto calibrateNode = this->getInput<Calibrate>();

				action(calibrateNode, calibrateControllerSession);
			}

			//----------
			void
				CalibrateController::up()
			{
				this->performAction([this](shared_ptr<Calibrate> calibrateNode
					, shared_ptr<Calibrate::CalibrateControllerSession> calibrateControllerSession)
					{
						calibrateNode->throwIfMissingAConnection<Installation>();
						auto installation = calibrateNode->getInput<Installation>();

						auto column = installation->getColumnByIndex(calibrateControllerSession->getColumnIndex(), false);

						auto moduleIndex = calibrateControllerSession->getModuleIndex();

						moduleIndex++;

						auto modules = column->getAllModules();
						if (moduleIndex >= modules.size()) {
							moduleIndex = 0;
						}

						calibrateControllerSession->setModuleIndex(moduleIndex);
					});
			}

			//----------
			void
				CalibrateController::down()
			{
				this->performAction([this](shared_ptr<Calibrate> calibrateNode
					, shared_ptr<Calibrate::CalibrateControllerSession> calibrateControllerSession)
					{
						calibrateNode->throwIfMissingAConnection<Installation>();
						auto installation = calibrateNode->getInput<Installation>();

						auto column = installation->getColumnByIndex(calibrateControllerSession->getColumnIndex(), false);

						auto moduleIndex = calibrateControllerSession->getModuleIndex();

						moduleIndex--;

						auto modules = column->getAllModules();

						if (moduleIndex < 0) {
							moduleIndex = modules.size() - 1;
						}

						calibrateControllerSession->setModuleIndex(moduleIndex);
					});
			}

			//----------
			void
				CalibrateController::left()
			{
				this->performAction([this](shared_ptr<Calibrate> calibrateNode
					, shared_ptr<Calibrate::CalibrateControllerSession> calibrateControllerSession)
					{
						calibrateNode->throwIfMissingAConnection<Installation>();
						auto installation = calibrateNode->getInput<Installation>();

						auto columnIndex = calibrateControllerSession->getColumnIndex();

						columnIndex--;

						if (columnIndex < 0) {
							auto columns = installation->getAllColumns();
							columnIndex = columns.size() - 1;
						}

						calibrateControllerSession->setColumnIndex(columnIndex);
					});
			}

			//----------
			void
				CalibrateController::right()
			{
				this->performAction([this](shared_ptr<Calibrate> calibrateNode
					, shared_ptr<Calibrate::CalibrateControllerSession> calibrateControllerSession)
					{
						calibrateNode->throwIfMissingAConnection<Installation>();
						auto installation = calibrateNode->getInput<Installation>();


						auto columnIndex = calibrateControllerSession->getColumnIndex();

						columnIndex++;

						auto columns = installation->getAllColumns();
						if (columnIndex >= columns.size()) {
							columnIndex = 0;
						}

						calibrateControllerSession->setColumnIndex(columnIndex);
					});
			}

			//----------
			void
				CalibrateController::x()
			{
				this->performAction([this](shared_ptr<Calibrate> calibrateNode
					, shared_ptr<Calibrate::CalibrateControllerSession> calibrateControllerSession)
					{
						calibrateNode->markDataPointGood(calibrateControllerSession);
					});
			}

			//----------
			void
				CalibrateController::circle()
			{
				this->performAction([this](shared_ptr<Calibrate> calibrateNode
					, shared_ptr<Calibrate::CalibrateControllerSession> calibrateControllerSession)
					{
						calibrateNode->clearModuleDataSetValues(calibrateControllerSession);
					});
			}

			//----------
			void
				CalibrateController::square()
			{
				this->performAction([this](shared_ptr<Calibrate> calibrateNode
					, shared_ptr<Calibrate::CalibrateControllerSession> calibrateControllerSession)
					{
						auto installation = calibrateNode->getInput<Installation>();
						if (installation) {
							auto module = installation->getModuleByIndices(calibrateControllerSession->getColumnIndex()
								, calibrateControllerSession->getModuleIndex()
								, true);
							if (module) {
								module->homeRoutine();
							}
						}
					});
			}

			//----------
			void
				CalibrateController::moveAxes(const glm::vec2 & movement)
			{
				auto calibrateControllerSession = this->calibrateControllerSession.lock();
				if (!calibrateControllerSession) {
					return;
				}

				this->throwIfMissingAConnection<Calibrate>();
				auto calibrateNode = this->getInput<Calibrate>();
				calibrateNode->moveDataPoint(calibrateControllerSession, movement);
			}
			
			//----------
			void
				CalibrateController::selectClosestModuleToMouseCursor()
			{
				auto calibrateControllerSession = this->calibrateControllerSession.lock();

				if (!calibrateControllerSession) {
					return;
				}

				this->throwIfMissingAConnection<Calibrate>();
				auto calibrate = this->getInput<Calibrate>();
				calibrate->throwIfMissingAConnection<Installation>();
				auto installation = calibrate->getInput<Installation>();

				auto selectedModulesByIndex = installation->getSelectedModulesByIndex();

				float closestDistance = std::numeric_limits<float>::max();
				bool foundAny = false;
				Data::Reworld::ColumnIndex closestColumnIndex;
				Data::Reworld::ColumnIndex closestModuleIndex;

				auto mouseWorldPosition = ofxRulr::Graph::World::X().getWorldStage()->getCamera().getCursorWorld();

				for (auto columnIt : selectedModulesByIndex) {
					for (auto moduleIt : columnIt.second) {
						auto module = moduleIt.second;
						auto modulePosition = module->getPosition();
						auto distance = glm::distance2(mouseWorldPosition, modulePosition);
						if (distance < closestDistance) {
							foundAny = true;
							closestDistance = distance;
							closestColumnIndex = columnIt.first;
							closestModuleIndex = moduleIt.first;
						}
					}
				}

				if (foundAny) {
					calibrateControllerSession->setColumnIndex(closestColumnIndex);
					calibrateControllerSession->setModuleIndex(closestModuleIndex);
				}
			}

			//----------
			void
				CalibrateController::rebuildPanel()
			{
				auto panel = this->panel;
				
				panel->clear();

				auto calibrateControllerSession = this->calibrateControllerSession.lock();
				if (calibrateControllerSession) {
					panel->addEditableValue<Data::Reworld::ColumnIndex>("Column", [calibrateControllerSession]() {
						return calibrateControllerSession->getColumnIndex();
						}, [calibrateControllerSession](string valueString) {
							if (!valueString.empty()) {
								calibrateControllerSession->setColumnIndex((Data::Reworld::ColumnIndex)ofToInt(valueString));
							}
							});

					panel->addEditableValue<Data::Reworld::ModuleIndex>("Module", [calibrateControllerSession]() {
						return calibrateControllerSession->getModuleIndex();
						}, [calibrateControllerSession](string valueString) {
							if (!valueString.empty()) {
								calibrateControllerSession->setModuleIndex((Data::Reworld::ModuleIndex)ofToInt(valueString));
							}
							});
				}
				else {
					panel->addTitle("Select a Calibrate node");
				}
			}
		}
	}
}
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

				ButtonState buttonState;
				{
					buttonState.up = remoteControlArgs.digital.y > 0;
					buttonState.down = remoteControlArgs.digital.y < 0;
					buttonState.left = remoteControlArgs.digital.x < 0;
					buttonState.right = remoteControlArgs.digital.x > 0;
				}

				try {

					if (buttonState.up && !this->priorButtonState.up) {
						this->up();
					}

					if (buttonState.down && !this->priorButtonState.down) {
						this->down();
					}

					if (buttonState.left && !this->priorButtonState.left) {
						this->left();
					}

					if (buttonState.right && !this->priorButtonState.right) {
						this->right();
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

				this->priorButtonState = buttonState;
			}

			//----------
			ofxCvGui::PanelPtr
				CalibrateController::getPanel()
			{
				return this->panel;
			}

			//----------
			void
				CalibrateController::up()
			{
				auto calibrateControllerSession = this->calibrateControllerSession.lock();
				if (!calibrateControllerSession) {
					return;
				}

				this->throwIfMissingAConnection<Calibrate>();
				auto calibrateNode = this->getInput<Calibrate>();
				calibrateNode->throwIfMissingAConnection<Installation>();
				auto installation = calibrateNode->getInput<Installation>();
				
				auto column = installation->getColumnByIndex(calibrateControllerSession->columnIndex, false);
				
				calibrateControllerSession->moduleIndex++;

				auto modules = column->getAllModules();
				if (calibrateControllerSession->moduleIndex >= modules.size()) {
					calibrateControllerSession->moduleIndex = 0;
				}
			}

			//----------
			void
				CalibrateController::down()
			{
				auto calibrateControllerSession = this->calibrateControllerSession.lock();
				if (!calibrateControllerSession) {
					return;
				}

				this->throwIfMissingAConnection<Calibrate>();
				auto calibrateNode = this->getInput<Calibrate>();
				calibrateNode->throwIfMissingAConnection<Installation>();
				auto installation = calibrateNode->getInput<Installation>();

				auto column = installation->getColumnByIndex(calibrateControllerSession->columnIndex, false);

				calibrateControllerSession->moduleIndex--;

				auto modules = column->getAllModules();
				if (calibrateControllerSession->moduleIndex < 0) {
					calibrateControllerSession->moduleIndex = modules.size() - 1;
				}
			}

			//----------
			void
				CalibrateController::left()
			{
				auto calibrateControllerSession = this->calibrateControllerSession.lock();
				if (!calibrateControllerSession) {
					return;
				}

				this->throwIfMissingAConnection<Calibrate>();
				auto calibrateNode = this->getInput<Calibrate>();
				calibrateNode->throwIfMissingAConnection<Installation>();
				auto installation = calibrateNode->getInput<Installation>();

				calibrateControllerSession->columnIndex--;
				if (calibrateControllerSession->columnIndex < 0) {
					auto columns = installation->getAllColumns();
					calibrateControllerSession->columnIndex = columns.size() - 1;
				}
			}

			//----------
			void
				CalibrateController::right()
			{
				auto calibrateControllerSession = this->calibrateControllerSession.lock();
				if (!calibrateControllerSession) {
					return;
				}

				this->throwIfMissingAConnection<Calibrate>();
				auto calibrateNode = this->getInput<Calibrate>();
				calibrateNode->throwIfMissingAConnection<Installation>();
				auto installation = calibrateNode->getInput<Installation>();

				calibrateControllerSession->columnIndex++;
				auto columns = installation->getAllColumns();
				if (calibrateControllerSession->columnIndex >= columns.size()) {
					calibrateControllerSession->columnIndex = 0;
				}
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
					calibrateControllerSession->columnIndex = closestColumnIndex;
					calibrateControllerSession->moduleIndex = closestModuleIndex;
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
						return calibrateControllerSession->columnIndex;
						}, [calibrateControllerSession](string valueString) {
							if (!valueString.empty()) {
								calibrateControllerSession->columnIndex = (Data::Reworld::ColumnIndex) ofToInt(valueString);
							}
							});

					panel->addEditableValue<Data::Reworld::ModuleIndex>("Module", [calibrateControllerSession]() {
						return calibrateControllerSession->moduleIndex;
						}, [calibrateControllerSession](string valueString) {
							if (!valueString.empty()) {
								calibrateControllerSession->moduleIndex = (Data::Reworld::ModuleIndex)ofToInt(valueString);
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
#include "pch_Plugin_Experiments.h"

namespace ofxRulr {
	namespace Nodes {
		namespace Experiments {
			namespace MirrorPlaneCapture {
				//---------
				NavigateToHalo::NavigateToHalo() {
					RULR_NODE_INIT_LISTENER;
				}

				//---------
				string NavigateToHalo::getTypeName() const {
					return "Halo::NavigateToHalo";
				}

				//---------
				void NavigateToHalo::init() {
					RULR_NODE_UPDATE_LISTENER
					RULR_NODE_INSPECTOR_LISTENER;

					this->addInput<Heliostats2>();
					this->addInput<SunTracker>();
					this->addInput<Item::View>("Viewer");
				}

				//---------
				void NavigateToHalo::update() {
					// Check update time
					if(this->parameters.schedule.enabled.get()) {
						auto timeSinceLastUpdate = chrono::system_clock::now() - this->lastUpdateTime;
						auto secondsSinceLastUpdate = chrono::duration_cast<chrono::seconds>(timeSinceLastUpdate);

						if(secondsSinceLastUpdate >= this->parameters.schedule.period.get()) {
							try {
								this->navigate();
							}
							RULR_CATCH_ALL_TO_ERROR;
						}
					}
				}

				//---------
				void NavigateToHalo::populateInspector(ofxCvGui::InspectArguments& inspectArgs) {
					auto inspector = inspectArgs.inspector;
					inspector->addButton("Navigate", [this]() {
						try {
							this->navigate();
						}
						RULR_CATCH_ALL_TO_ALERT;
						});
				}

				//---------
				void NavigateToHalo::navigate() {
					this->throwIfMissingAnyConnection<Heliostats2>();
					auto heliostatsNode = this->getInput<Heliostats2>();
					auto heliostats = heliostatsNode->getHeliostats();

					auto viewNode = this->getInput<Item::View>();
					auto view = viewNode->getViewInWorldSpace();

					auto solverSettings = Solvers::HeliostatActionModel::Navigator::defaultSolverSettings();
					solverSettings.printReport = this->parameters.solver.printReport.get();
					solverSettings.options.minimizer_progress_to_stdout = this->parameters.solver.printReport.get();
					solverSettings.num_max_iterations = this->parameters.solver.maxIterations.get();
					solverSettings.functionTolerance = this->parameters.solver.functionTolerance.get();
					
					for (auto heliostat : heliostats) {
						heliostat->navigateToReflectPointToPoint(cursorInWorld
							, cursorInWorld
							, solverSettings
							, false);
					}
				}
			}
		}
	}
}
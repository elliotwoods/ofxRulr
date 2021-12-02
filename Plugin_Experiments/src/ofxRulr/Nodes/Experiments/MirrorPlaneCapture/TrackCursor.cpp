#include "pch_Plugin_Experiments.h"

namespace ofxRulr {
	namespace Nodes {
		namespace Experiments {
			namespace MirrorPlaneCapture {
				//---------
				TrackCursor::TrackCursor() {
					RULR_NODE_INIT_LISTENER;
				}

				//---------
				string TrackCursor::getTypeName() const {
					return "Halo::TrackCursor";
				}

				//---------
				void TrackCursor::init() {
					RULR_NODE_INSPECTOR_LISTENER;
					RULR_NODE_UPDATE_LISTENER;

					this->manageParameters(this->parameters);

					this->addInput<Heliostats2>();
				}

				//---------
				void TrackCursor::update() {
					{
						bool shouldPerform = isActive(this, this->parameters.perform);
						if (shouldPerform) {
							try {
								this->track();
							}
							RULR_CATCH_ALL_TO_ERROR;
						}
					}
					
				}

				//---------
				void TrackCursor::populateInspector(ofxCvGui::InspectArguments& inspectArgs) {
					auto inspector = inspectArgs.inspector;
					inspector->addButton("Track to cursor", [this]() {
						try {
							this->track();
						}
						RULR_CATCH_ALL_TO_ALERT;
						});
				}

				//---------
				void TrackCursor::track() {
					this->throwIfMissingAConnection<Heliostats2>();
					auto heliostatsNode = this->getInput<Heliostats2>();
					auto heliostats = heliostatsNode->getHeliostats();

					auto cursorInWorld = Graph::World::X().getWorldStage()->getCursorWorld();

					auto solverSettings = Solvers::HeliostatActionModel::Navigator::defaultSolverSettings();
					solverSettings.printReport = this->parameters.navigator.printReport.get();
					solverSettings.options.minimizer_progress_to_stdout = this->parameters.navigator.printReport.get();

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
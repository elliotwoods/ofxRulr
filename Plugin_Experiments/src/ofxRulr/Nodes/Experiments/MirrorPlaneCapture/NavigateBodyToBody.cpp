#include "pch_Plugin_Experiments.h"

namespace ofxRulr {
	namespace Nodes {
		namespace Experiments {
			namespace MirrorPlaneCapture {
				//----------
				NavigateBodyToBody::NavigateBodyToBody() {
					RULR_NODE_INIT_LISTENER;
				}

				//----------
				string NavigateBodyToBody::getTypeName() const {
					return "Halo::NavigateBodyToBody";
				}

				//----------
				void NavigateBodyToBody::init() {
					RULR_NODE_INSPECTOR_LISTENER;

					this->addInput<Heliostats2>();
					this->addInput<Item::RigidBody>("Body A");
					this->addInput<Item::RigidBody>("Body B");
				}

				//----------
				void NavigateBodyToBody::populateInspector(ofxCvGui::InspectArguments& inspectArgs) {
					auto inspector = inspectArgs.inspector;

					inspector->addButton("Navigate", [this]() {
						try {
							this->navigate();
						}
						RULR_CATCH_ALL_TO_ALERT;
						}, ' ')->setHeight(100.0f);
				}

				//----------
				void NavigateBodyToBody::navigate() {
					this->throwIfMissingAnyConnection();

					auto heliostatsNode = this->getInput<Heliostats2>();

					auto bodyA = this->getInput<Item::RigidBody>("Body A");
					auto bodyB = this->getInput<Item::RigidBody>("Body B");

					auto positionA = bodyA->getPosition();
					auto positionB = bodyB->getPosition();

					auto solverSettings = ofxRulr::Solvers::HeliostatActionModel::Navigator::defaultSolverSettings();

					solverSettings.options.max_num_iterations = this->parameters.maxIterations.get();
					solverSettings.printReport = this->parameters.printReport.get();
					solverSettings.options.minimizer_progress_to_stdout = this->parameters.printReport.get();

					auto heliostats = heliostatsNode->getHeliostats();
					for (auto heliostat : heliostats) {
						heliostat->navigateToReflectPointToPoint(positionA
							, positionB
							, solverSettings);
					}

					if (this->parameters.pushValues) {
						heliostatsNode->pushStale(true, true);
					}
				}
			}
		}
	}
}
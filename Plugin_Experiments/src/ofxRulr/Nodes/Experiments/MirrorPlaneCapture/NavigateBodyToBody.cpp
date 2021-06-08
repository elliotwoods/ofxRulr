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
						}, OF_KEY_RETURN)->setHeight(100.0f);
				}

				//----------
				void NavigateBodyToBody::navigate() {
					this->throwIfMissingAnyConnection();

					auto heliostatsNode = this->getInput<Heliostats2>();

					auto bodyA = this->getInput<Item::RigidBody>("Body A");
					auto bodyB = this->getInput<Item::RigidBody>("Body B");

					glm::vec3 positionA = bodyA->getPosition();
					glm::vec3 positionB = bodyB->getPosition();

					// Do board center if it's a board
					{
						{
							auto board = dynamic_pointer_cast<Item::BoardInWorld>(bodyA);
							if (board) {
								auto worldPoints = board->getAllWorldPoints();
								glm::vec3 center(0, 0, 0);
								for (const auto& worldPoint : worldPoints) {
									center += worldPoint;
								}
								center /= worldPoints.size();
								positionA = center;
							}
						}

						{
							auto board = dynamic_pointer_cast<Item::BoardInWorld>(bodyB);
							if (board) {
								auto worldPoints = board->getAllWorldPoints();
								glm::vec3 center(0, 0, 0);
								for (const auto& worldPoint : worldPoints) {
									center += worldPoint;
								}
								center /= worldPoints.size();
								positionB = center;
							}
						}
					}
					auto solverSettings = ofxRulr::Solvers::HeliostatActionModel::Navigator::defaultSolverSettings();

					solverSettings.options.max_num_iterations = this->parameters.maxIterations.get();
					solverSettings.printReport = this->parameters.printReport.get();
					solverSettings.options.minimizer_progress_to_stdout = this->parameters.printReport.get();

					auto heliostats = heliostatsNode->getHeliostats();
					for (auto heliostat : heliostats) {
						heliostat->navigateToReflectPointToPoint(positionA
							, positionB
							, solverSettings
						, this->parameters.throwIfOutsideRange.get());
					}

					if (this->parameters.pushValues) {
						heliostatsNode->pushStale(true, false);
					}
				}
			}
		}
	}
}
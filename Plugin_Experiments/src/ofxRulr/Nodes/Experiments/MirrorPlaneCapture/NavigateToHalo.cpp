#include "pch_Plugin_Experiments.h"

// from https://stackoverflow.com/questions/49968720/find-tangent-points-in-a-circle-from-a-point/49981991
void getTangents(const glm::vec2 & center
	, float radius
	, const glm::vec2 & point
	, glm::vec2 & leftTangent
	, glm::vec2 & rightTangent) {
	auto b = glm::distance(center, point); // hyp
	auto theta = acos(radius / b); //angle theta
	auto d = atan2(point.y - center.y, point.x - center.x); // direction angle of point P from C
	auto thetaLeftTangent = d - theta; // direction angle of point T1 from C
	auto thetaRightTangent = d + theta; //direction angle of point T2 from C

	leftTangent.x = center.x + radius * cos(d1);
	leftTangent.y = center.y + radius * sin(d1);
	rightTangent.x = center.x + radius * cos(d2);
	rightTangent.y = center.y + radius * sin(d2);
}

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
					this->addInput<Halo>();
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
					this->throwIfMissingAnyConnection();
					auto heliostatsNode = this->getInput<Heliostats2>();
					auto heliostats = heliostatsNode->getHeliostats();

					auto viewNode = this->getInput<Item::View>();
					auto view = viewNode->getViewInWorldSpace();

					auto sunTracker = this->getInput<SunTracker>();
					auto solarVector = sunTracker->getSolarVectorWorldSpace(chrono::system_clock::now());

					auto solverSettings = Solvers::HeliostatActionModel::Navigator::defaultSolverSettings();
					{
						solverSettings.printReport = this->parameters.solver.printReport.get();
						solverSettings.options.minimizer_progress_to_stdout = this->parameters.solver.printReport.get();
						solverSettings.num_max_iterations = this->parameters.solver.maxIterations.get();
						solverSettings.functionTolerance = this->parameters.solver.functionTolerance.get();
					}

					const auto haloCenter = halo->getPosition();
					glm::vec2 haloCenterInView = view.worldToPixels(haloCenter);

					float haloRadiusInView;
					{
						auto haloTopWorld = haloCenter + glm::vec3(0, halo->getRadius(), 0);
						auto haloTopInView = view.worldToPixels(haloTopWorld);
						haloRadiusInView = glm::distance(haloTopInView, haloCenterInView)
					}

					auto haloTransform = halo->getTransform();
					auto haloNormal = Utils::applyTransform(haloTransform, {0, 0, 1}) - haloCenter;
					ofxRay::Plane haloPlane(haloCenter, haloNormal);
					{
						haloPlane.infinite = true;
					}

					for (auto heliostat : heliostats) {
						const auto & heliostatPosition = heliostat->parameters.hamParameters.position.get();
						// Calculate the target point in view space
						glm::vec2 targetPointViewSpace;
						{
							glm::vec2 heliostatInView = view.worldToPixels(heliostatPosition);
							glm::vec2 leftTangent, rightTangent;
							getTangents(heliostatInView
								, haloRadiusInView
								, heliostatInView
								, leftTangent
								, rightTangent);

							targetPointViewSpace = heliostat->parameters.rightTangent.get()
								? rightTangent
								: leftTangent;
						}

						// Unproject to a ray from the viewer
						auto targetRayFromView = view.castPixel(targetPointViewSpace);

						// Intersect that ray with the halo plane to get a 3D point in world space
						auto targetWorldSpace = haloPlane.intersect(targetRayFromView);

						heliostat->navigateToReflectVectorToPoint(solarVector
							, targetWorldSpace
							, solverSettings
							, false);
					}
				}
			}
		}
	}
}
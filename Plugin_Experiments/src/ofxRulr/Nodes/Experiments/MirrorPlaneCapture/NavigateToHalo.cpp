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

	leftTangent.x = center.x + radius * cos(thetaLeftTangent);
	leftTangent.y = center.y + radius * sin(thetaLeftTangent);
	rightTangent.x = center.x + radius * cos(thetaRightTangent);
	rightTangent.y = center.y + radius * sin(thetaRightTangent);
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
					RULR_NODE_UPDATE_LISTENER;
					RULR_NODE_INSPECTOR_LISTENER;
					RULR_NODE_DRAW_WORLD_LISTENER;

					this->manageParameters(this->parameters);

					this->addInput<Heliostats2>();
					this->addInput<SunTracker>();
					this->addInput<Item::View>("Viewer");
					this->addInput<Halo>();
				}

				//---------
				void NavigateToHalo::update() {
					// Auto navigate
					{
						bool autoNavigate = false;

						// Check update time
						if (this->parameters.schedule.update.enabled.get()) {
							auto timeSinceLastUpdate = chrono::system_clock::now() - this->lastUpdateTime;
							auto secondsSinceLastUpdate = chrono::duration_cast<chrono::seconds>(timeSinceLastUpdate).count();

							if (secondsSinceLastUpdate >= this->parameters.schedule.update.period.get()) {
								autoNavigate = true;
							}
						}

						// On halo change
						{
							if (this->parameters.onHaloChange.get()) {
								auto halo = this->getInput<Halo>();
								if (halo) {
									if (halo->isSettingsNew()) {
										autoNavigate = true;
									}
								}
							}
						}

						if (autoNavigate) {
							try {
								this->navigate();
							}
							RULR_CATCH_ALL_TO_ERROR;
						}
					}

					// Shutdown
					if (this->parameters.schedule.shutdown.enabled.get()) {
						// check if it's been over 5s since we last called this command
						if (chrono::system_clock::now() - this->lastShutdownRequest > chrono::milliseconds(1000 * this->parameters.schedule.shutdown.shutdownDelay.get())) {
							// check if we should shutdown
							if (this->getShouldShutdown()) {
								try {
									this->shutdown();
								}
								RULR_CATCH_ALL_TO_ERROR;
							}
						}
					}
				}

				//---------
				void NavigateToHalo::drawWorldStage() {
					if (this->parameters.draw.targets.get()) {
						for (const auto& it : this->targetsCache) {
							ofxCvGui::Utils::drawTextAnnotation(it.first, it.second.targetPosition, this->getColor());
						}
					}

					ofPushStyle();
					{
						ofSetColor(this->getColor());
						if (this->parameters.draw.lines.get()) {
							for (const auto& it : this->targetsCache) {
								ofDrawLine(it.second.heliostatPosition, it.second.targetPosition);
							}
						}
					}
					ofPopStyle();

					ofPushStyle();
					{
						ofSetColor(ofColor(255, 200, 200));
						if (this->parameters.draw.solarIncidentVectors.get()) {
							for (const auto& it : this->targetsCache) {
								auto& cachedTarget = it.second;
								ofDrawArrow(cachedTarget.heliostatPosition - cachedTarget.solarIncidentVector, cachedTarget.heliostatPosition, 0.01);
							}
						}
					}
					ofPopStyle();
				}

				//---------
				void NavigateToHalo::populateInspector(ofxCvGui::InspectArguments& inspectArgs) {
					auto inspector = inspectArgs.inspector;
					inspector->addButton("Navigate", [this]() {
						try {
							this->navigate();
						}
						RULR_CATCH_ALL_TO_ALERT;
						}, OF_KEY_RETURN)->setHeight(100.0f);

					inspector->addButton("Set alternate targets", [this]() {
						try {
							this->setAlternateTangents();
						}
						RULR_CATCH_ALL_TO_ALERT;
						});

					inspector->addLiveValue<float>("Seconds since last update", [this]() {
						auto timeSinceLastUpdate = chrono::system_clock::now() - this->lastUpdateTime;
						return (float)chrono::duration_cast<chrono::seconds>(timeSinceLastUpdate).count();
						});

					inspector->addButton("Navigate to sun", [this]() {
						try {
							this->navigateToSun();
						}
						RULR_CATCH_ALL_TO_ALERT
						});

					inspector->addIndicatorBool("Should shutdown", [this]() {
						return this->getShouldShutdown();
						});

					inspector->addButton("Shutdown", [this]() {
						this->shutdown();
						});
				}

				//---------
				void NavigateToHalo::navigate() {
					this->throwIfMissingAnyConnection();
					auto heliostatsNode = this->getInput<Heliostats2>();
					auto heliostats = heliostatsNode->getHeliostats();
					auto halo = this->getInput<Halo>();

					auto viewNode = this->getInput<Item::View>();
					auto view = viewNode->getViewInWorldSpace();

					this->targetsCache.clear();

					auto sunTracker = this->getInput<SunTracker>();
					auto solarIncidentVector = - sunTracker->getSolarVectorWorldSpace(chrono::system_clock::now());

					auto solverSettings = Solvers::HeliostatActionModel::Navigator::defaultSolverSettings();
					{
						solverSettings.printReport = this->parameters.solver.printReport.get();
						solverSettings.options.minimizer_progress_to_stdout = this->parameters.solver.printReport.get();
						solverSettings.options.max_num_iterations = this->parameters.solver.maxIterations.get();
						solverSettings.options.function_tolerance = this->parameters.solver.functionTolerance.get();
					}

					const auto haloCenter = halo->getPosition();
					glm::vec2 haloCenterInView = view.getScreenCoordinateOfWorldPosition(haloCenter);

					float haloRadiusInView;
					{
						auto haloTopWorld = haloCenter + glm::vec3(0, halo->getRadius(), 0);
						glm::vec2 haloTopInView = view.getScreenCoordinateOfWorldPosition(haloTopWorld);
						haloRadiusInView = glm::distance(haloTopInView, haloCenterInView);
					}

					auto haloTransform = halo->getTransform();
					auto haloNormal = Utils::applyTransform(haloTransform, {0, 0, 1}) - haloCenter;
					ofxRay::Plane haloPlane(haloCenter, haloNormal);
					{
						haloPlane.setInfinite(true);
					}

					for (auto heliostat : heliostats) {
						auto heliostatPosition = heliostat->parameters.hamParameters.position.get();
						// Calculate the target point in view space
						glm::vec2 targetPointViewSpace;
						{
							glm::vec2 heliostatInView = view.getScreenCoordinateOfWorldPosition(heliostatPosition);
							glm::vec2 leftTangent, rightTangent;
							getTangents(haloCenterInView
								, haloRadiusInView
								, heliostatInView
								, leftTangent
								, rightTangent);

							targetPointViewSpace = heliostat->parameters.rightTangent.get()
								? rightTangent
								: leftTangent;
						}

						// Unproject to a ray from the viewer
						auto targetRayFromView = view.castPixel(targetPointViewSpace, false);

						// Intersect that ray with the halo plane to get a 3D point in world space
						glm::vec3 targetWorldSpace;
						haloPlane.intersect(targetRayFromView, targetWorldSpace);

						// Cache target
						CachedTarget cachedTarget;
						{
							cachedTarget.heliostatPosition = heliostatPosition;
							cachedTarget.targetPosition = targetWorldSpace;
							cachedTarget.solarIncidentVector = solarIncidentVector;
						};

						this->targetsCache.emplace(heliostat->getName(), cachedTarget);

						heliostat->navigateToReflectVectorToPoint(solarIncidentVector
							, targetWorldSpace
							, solverSettings
							, false);
					}

					this->lastUpdateTime = chrono::system_clock::now();
				}

				//---------
				void NavigateToHalo::navigateToSun() {
					this->throwIfMissingAnyConnection();
					auto heliostatsNode = this->getInput<Heliostats2>();
					auto heliostats = heliostatsNode->getHeliostats();

					auto sunTracker = this->getInput<SunTracker>();
					auto solarIncidentVector = -sunTracker->getSolarVectorWorldSpace(chrono::system_clock::now());

					auto solverSettings = Solvers::HeliostatActionModel::Navigator::defaultSolverSettings();
					{
						solverSettings.printReport = this->parameters.solver.printReport.get();
						solverSettings.options.minimizer_progress_to_stdout = this->parameters.solver.printReport.get();
						solverSettings.options.max_num_iterations = this->parameters.solver.maxIterations.get();
						solverSettings.options.function_tolerance = this->parameters.solver.functionTolerance.get();
					}

					for (auto heliostat : heliostats) {
						auto heliostatPosition = heliostat->parameters.hamParameters.position.get();
						heliostat->navigateToNormal(-solarIncidentVector, solverSettings, true);
					}

					this->lastUpdateTime = chrono::system_clock::now();
				}

				//---------
				void NavigateToHalo::setAlternateTangents() {
					this->throwIfMissingAConnection<Heliostats2>();
					auto helisotatsNode = this->getInput<Heliostats2>();
					auto heliostats = helisotatsNode->getHeliostats();
					bool right;
					for (auto heliostat : heliostats) {

						heliostat->parameters.rightTangent.set(right);
						right ^= true;
					}
				}

				//---------
				bool
					NavigateToHalo::getShouldShutdown() const
				{
					// Get the current time
					std::time_t currentTime = std::time(nullptr);

					// Convert to a tm struct
					std::tm* localTime = std::localtime(&currentTime);

					// Calculate hour as a float
					float hourAsFloat = localTime->tm_hour + (localTime->tm_min / 60.0f);

					return hourAsFloat > this->parameters.schedule.shutdown.hour.get();
				}

				//---------
				void
					NavigateToHalo::shutdown()
				{
					// Disable navigate scheduler
					this->parameters.schedule.update.enabled.set(false);

					// Home everything
					auto heliostatsNode = this->getInput<Heliostats2>();
					if (heliostatsNode) {
						auto heliostats = heliostatsNode->getHeliostats();
						for (auto heliostat : heliostats) {
							heliostat->parameters.servo1.angle.set(0.0f);
							heliostat->parameters.servo2.angle.set(0.0f);
						}
						try {
							heliostatsNode->pushAll(true);
						}
						RULR_CATCH_ALL_TO_ERROR;
					}
										
					// Perform the shutdown in 120s time
					auto command = "shutdown /f /s /t " + ofToString(this->parameters.schedule.shutdown.shutdownDelay.get());
					system(command.c_str());

					this->lastShutdownRequest = chrono::system_clock::now();
				}
			}
		}
	}
}
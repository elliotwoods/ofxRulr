#include "pch_Plugin_Experiments.h"
#include "SolTrack.h"

namespace ofxRulr {
	namespace Nodes {
		namespace Experiments {
			namespace MirrorPlaneCapture {
				//---------
				SunTracker::SunTracker() {
					RULR_NODE_INIT_LISTENER;
				}

				//---------
				string SunTracker::getTypeName() const {
					return "Halo::SunTracker";
				}

				//---------
				void SunTracker::init() {
					RULR_NODE_INSPECTOR_LISTENER;
					RULR_RIGIDBODY_DRAW_OBJECT_LISTENER;

					this->light.setDirectional();
				}

				//---------
				void SunTracker::update() {
					
				}

				//---------
				void SunTracker::drawObject() {
					auto solarVectorObjectSpace = this->getSolarVectorObjectSpace(chrono::system_clock::now());

					if(this->parameters.renderInternal.get()) {
						glEnable(GL_CULL_FACE);
						glPushAttrib(GL_POLYGON_BIT);

						this->light.lookAt(this->getSolarVectorObject());
						this->light.enable();
						{
							ofDrawSphere(this->parameters.draw.sphereRadius.get());
						}
						this->light.disable();
						glPopAttrib();
						glDisable(GL_CULL_FACE);

						ofDrawArrow({0, 0, 0}
							, solarVectorObjectSpace * this->parameters.draw.sphereRadius.get()
							, 0.1f);
					}
					else {
						this->light.lookAt(this->getSolarVectorObject());
						this->light.enable();
						{
							ofDrawSphere(this->parameters.draw.sphereRadius.get());
						}
						this->light.disable();

						ofDrawArrow(solarVectorObjectSpace * this->parameters.draw.sphereRadius.get() * 2.0f
							, solarVectorObjectSpace * this->parameters.draw.sphereRadius.get()
							, 0.1f);
					}
				}
	
				//---------
				glm::vec SunTracker::getAzimuthAltitude(const chrono::system_clock::time_point& timePoint) {
					Time solTrackTime;
					{
						timePoint_C = system_clock::to_time_t(timePoint);
						tm utc_tm = *gmtime(&timePoint_C);
						
						solTrackTime.year = utc_tm.tm_year;
						solTrackTime.month = utc_tm.tm_mon;
						solTrackTime.day = utc_tm.tm_mday;
						solTrackTime.hour = utc_tm.tm_hour;
						solTrackTime.minute = utc_tm.tm_min;
						solTrackTime.second = utc_tm.tm_sec;
					}

					Location solTrackLocation;
					{
						solTrackLocation.latitude = this->parameters.location.latitude.get();
						solTrackLocation.longitude = this->parameters.location.longitude.get();
						solTrackLocation.pressure = this->parameters.location.pressure.get();
						solTrackLocation.temperature = 273.15 + this->parameters.location.temperature.get();
					}

					Position solTrackResult;
					SolTrack(solTracktime
						, solTrackLocation
						, &solTrackResult
						, 1
						, 1
						, 1
						, 0);
					
					float thetaClockwiseFromNorth = solTrackResult.azimuthRefract * DEG_TO_RAD;
					float theta = (PI / 2.0) - thetaClockwiseFromNorth;

					float thi = solTrackResult.altitudeRefract * DEG_TO_RAD;

					return {
						theta
						, thi
					};
				}

				//---------
				glm::vec3 SunTracker::getSolarVectorObjectSpace(const chrono::system_clock::time_point& timePoint) const {
					auto thetaThi = this->getAzimuthAltitude(timePoint);

					return 	glm::quaternion::rotate(thetaThi.x, glm::vec3(0,1,0))
								* glm::quaternion::rotate(thetaThi.y, glm::vec3(0,0,-1))
								* glm::vec3 (-1.0f, 0.0f, 0.0f);
				}

				//---------
				glm::vec3 SunTracker::getSolarVectorWorldSpace(const chrono::system_clock::time_point& timePoint) const {
					return this->getOrientation() * this->getSolarVectorObjectSpace(timePoint);
				}

				//---------
				void SunTracker::populateInspector(ofxCvGui::InspectArguments& inspectArgs) {
					auto inspector = inspectArgs.inspector;
					inspector->addButton("Track to cursor", [this]() {
						try {
							this->track();
						}
						RULR_CATCH_ALL_TO_ALERT;
						});
				}

				//---------
				void SunTracker::track() {
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
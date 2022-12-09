#include "pch_Plugin_Experiments.h"

#include "SolTrack/SolTrack.h"

namespace ofxRulr {
	namespace Nodes {
		namespace Experiments {
			namespace MirrorPlaneCapture {
				//---------
				SunTracker::SunTracker() {
					RULR_NODE_INIT_LISTENER;
					this->setColor(ofColor(255, 255, 200));
				}

				//---------
				string SunTracker::getTypeName() const {
					return "Halo::SunTracker";
				}

				//---------
				void SunTracker::init() {
					RULR_RIGIDBODY_DRAW_OBJECT_LISTENER;
					RULR_NODE_UPDATE_LISTENER;
					RULR_NODE_INSPECTOR_LISTENER;

					this->manageParameters(this->parameters);
					this->light.setDirectional();
					this->light.setAmbientColor({ 0 });
					this->light.setDiffuseColor({1.0f});
				}

				//---------
				void SunTracker::update() {
					if (this->parameters.debug.offsetTimeEnabled.get()) {
						this->solarVectorObjectSpaceNow = this->getSolarVectorObjectSpace(this->getOffsetTime());
					}
					else {
						this->solarVectorObjectSpaceNow = this->getSolarVectorObjectSpace(chrono::system_clock::now());
					}
				}

				//---------
				void SunTracker::drawObject() {
					ofPushStyle();
					{
						ofSetColor(this->getColor());

						if (this->parameters.draw.renderInternal.get()) {
							Utils::Graphics::glEnable(GL_CULL_FACE);
							Utils::Graphics::glPushAttrib(GL_POLYGON_BIT);

							//this->light.lookAt(this->solarVectorObjectSpaceNow);
							this->light.enable();
							{
								ofPushStyle();
								{
									ofSetSphereResolution(this->parameters.draw.sphereResolution.get());
									ofDrawSphere(this->parameters.draw.sphereRadius.get());
								}
								ofPopStyle();
							}
							this->light.disable();

							Utils::Graphics::glPopAttrib();
							Utils::Graphics::glDisable(GL_CULL_FACE);

							ofDrawArrow({ 0, 0, 0 }
								, this->solarVectorObjectSpaceNow * this->parameters.draw.sphereRadius.get()
								, 0.1f);
						}
						else {
							this->light.lookAt(this->solarVectorObjectSpaceNow);
							this->light.enable();
							{
								ofDrawSphere(this->parameters.draw.sphereRadius.get());
							}
							this->light.disable();

							ofDrawArrow(this->solarVectorObjectSpaceNow * (this->parameters.draw.sphereRadius.get() + this->parameters.draw.arrowLength.get())
								, this->solarVectorObjectSpaceNow * this->parameters.draw.sphereRadius.get()
								, 0.1f);
						}
					}
					ofPopStyle();
				}

				//---------
				void SunTracker::populateInspector(ofxCvGui::InspectArguments& args) {
					auto inspector = args.inspector;
					inspector->addLiveValue<glm::vec3>("Solar vector (object)", [this]() {
						return this->solarVectorObjectSpaceNow;
						});

					inspector->addLiveValue<string>("Offset time", [this]() {
						time_t tt = chrono::system_clock::to_time_t(this->getOffsetTime());
						tm local_tm = *localtime(&tt);

						stringstream ss;
						ss << local_tm.tm_hour << ":" << local_tm.tm_min;
						return ss.str();
						});
				}


				//---------
				glm::vec2 SunTracker::getAzimuthAltitude(const chrono::system_clock::time_point& timePoint) const {
					Time solTrackTime;
					{
						auto timePoint_C = chrono::system_clock::to_time_t(timePoint);
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
						solTrackLocation.pressure = this->parameters.location.atmosphericPressure.get();
						solTrackLocation.temperature = 273.15 + this->parameters.location.temperature.get();
					}

					Position solTrackResult;
					SolTrack(solTrackTime
						, solTrackLocation
						, &solTrackResult
						, 1
						, 1
						, 1
						, 0);

					float thetaClockwiseFromNorth = solTrackResult.azimuthRefract * DEG_TO_RAD;
					float theta = - thetaClockwiseFromNorth;

					float thi = solTrackResult.altitudeRefract * DEG_TO_RAD;

					return {
						theta
						, thi
					};
				}

				//---------
				glm::vec3 SunTracker::getSolarVectorObjectSpace(const chrono::system_clock::time_point& timePoint) const {
					auto thetaThi = this->getAzimuthAltitude(timePoint);


					auto rotation = glm::rotate(thetaThi.x, glm::vec3(0, -1, 0))
						* glm::rotate(thetaThi.y, glm::vec3(0, 0, 1));

					return Utils::applyTransform(glm::mat4(rotation), glm::vec3(-1.0f, 0.0f, 0.0f));
				}

				//---------
				glm::vec3 SunTracker::getSolarVectorWorldSpace(const chrono::system_clock::time_point& timePoint) const {
					return Utils::applyTransform(glm::mat4(this->getRotationQuat()), this->getSolarVectorObjectSpace(timePoint));
				}

				//---------
				chrono::system_clock::time_point SunTracker::getOffsetTime() const {
					auto offset = chrono::minutes((int)(this->parameters.debug.offsetHours.get() * 60.0f));
					auto now = chrono::system_clock::now();
					return now + offset;
				}

			}
		}
	}
}
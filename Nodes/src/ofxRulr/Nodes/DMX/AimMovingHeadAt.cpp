#include "pch_RulrNodes.h"
#include "AimMovingHeadAt.h"

#include "ofxRulr/Nodes/Item/RigidBody.h"
#include "ofxRulr/Nodes/DMX/MovingHead.h"

#include "ofxCvGui/Widgets/Toggle.h"
#include "ofxCvGui/Widgets/Slider.h"
#include "ofxCvGui/Widgets/Title.h"
#include "ofxCvGui/Widgets/LiveValue.h"

#include "ofxRulr/Utils/PolyFit.h"

#include "ofxCvMin.h"

using namespace ofxCvGui;
using namespace cv;

namespace ofxRulr {
	namespace Nodes {
		namespace DMX {
			//----------
			AimMovingHeadAt::AimMovingHeadAt() {
				RULR_NODE_INIT_LISTENER;
			}

			//----------
			void AimMovingHeadAt::init() {
				RULR_NODE_UPDATE_LISTENER;
				RULR_NODE_INSPECTOR_LISTENER;
				RULR_NODE_SERIALIZATION_LISTENERS;

				this->addInput<MovingHead>();
				this->addInput<Item::RigidBody>("Target");

				this->ignoreBlankTransform.set("Ignore blank transform", true);

				{
					this->objectPositionOffset[0].set("Position offset X", 0.0f, -1.0f, 1.0f);
					this->objectPositionOffset[1].set("Position offset Y", 0.0f, -1.0f, 1.0f);
					this->objectPositionOffset[2].set("Position offset Z", 0.0f, -1.0f, 1.0f);
				}

				//prediction
				{
					this->steveJobs.enabled.set("Enabled", false);
					this->steveJobs.steps.set("Steps", 10, 0, 1000);
					this->steveJobs.minimumVelocity.set("Minimum velocity [m/s]", 1.0f, 0.0f, 20.0f);
					this->steveJobs.maximumAcceleration.set("Maximum acceleration [m/s^2]", 1.0f, 0.0f, 10.0f);
					this->steveJobs.kalmanFilter = KalmanFilter(6, 3, 0);
					this->steveJobs.measurmentVector = Mat_<float>(3, 1);
					this->steveJobs.isTracking = false;
					this->steveJobs.velocityOK = false;
					this->steveJobs.accelerationOK = false;
				}
			}

			//----------
			string AimMovingHeadAt::getTypeName() const {
				return "DMX::AimMovingHeadAtTarget";
			}

			//----------
			void AimMovingHeadAt::update() {
				auto movingHead = this->getInput<MovingHead>();
				auto target = this->getInput<Item::RigidBody>("Target");
				if (target && movingHead) {
					try {
						//check if transform is blank before using it (blank can be an indicator of bad tracking)
						if (this->ignoreBlankTransform) {
							if (target->getTransform() == glm::mat4(1.0f)) {
								this->steveJobs.isTracking = false;
								throw(Exception("Target has no position data"));
							}
						}

						//get either the raw position or the predicted position
						glm::vec3 aimFor;
						if(this->steveJobs.enabled) {
							aimFor = this->doASteveJobs(target->getPosition());
						}
						else {
							aimFor = target->getPosition();
						}

						//calculate the position offset in target coords
						const auto objectRotation = target->getRotationQuat();
						const auto offset = objectRotation * this->getObjectPositionOffset();

						//perform the lookAt
						movingHead->lookAt(aimFor + offset);
					}
					RULR_CATCH_ALL_TO_ERROR;
				}
				else {
					this->steveJobs.isTracking = false;
				}
			}

			//----------
			void AimMovingHeadAt::serialize(nlohmann::json & json) {
				Utils::serialize(json, this->ignoreBlankTransform);

				for (int i = 0; i < 3; i++) {
					Utils::serialize(json, this->objectPositionOffset[i]);
				}

				auto & jsonPrediction = json["prediction"];
				{
					Utils::serialize(jsonPrediction, this->steveJobs.enabled);
					Utils::serialize(jsonPrediction, this->steveJobs.steps);
					Utils::serialize(jsonPrediction, this->steveJobs.minimumVelocity);
					Utils::serialize(jsonPrediction, this->steveJobs.maximumAcceleration);
				}
			}

			//----------
			void AimMovingHeadAt::deserialize(const nlohmann::json & json) {
				Utils::deserialize(json, this->ignoreBlankTransform);

				for (int i = 0; i < 3; i++) {
					Utils::deserialize(json, this->objectPositionOffset[i]);
				}

				const auto & jsonPrediction = json["prediction"];
				{
					Utils::deserialize(jsonPrediction, this->steveJobs.enabled);
					Utils::deserialize(jsonPrediction, this->steveJobs.steps);
					Utils::deserialize(jsonPrediction, this->steveJobs.minimumVelocity);
					Utils::deserialize(jsonPrediction, this->steveJobs.maximumAcceleration);
				}
			}

			//----------
			void AimMovingHeadAt::populateInspector(ofxCvGui::InspectArguments & inspectArguments) {
				auto inspector = inspectArguments.inspector;
				
				inspector->add(new Widgets::Toggle(this->ignoreBlankTransform));

				inspector->add(new Widgets::Title("Position offset", Widgets::Title::Level::H2));
				for (int i = 0; i < 3; i++) {
					inspector->add(new Widgets::Slider(this->objectPositionOffset[i]));
				}

				inspector->add(new Widgets::Title("Steve Jobs Mode", Widgets::Title::Level::H2));
				{
					inspector->add(new Widgets::Title("Prediction", Widgets::Title::Level::H3));
					inspector->add(new Widgets::Toggle(this->steveJobs.enabled));
					inspector->add(new Widgets::Slider(this->steveJobs.steps));

					inspector->add(new Widgets::Title("Velocity", Widgets::Title::Level::H3));
					{
						inspector->add(new Widgets::Slider(this->steveJobs.minimumVelocity));
						auto velocityWidget = new Widgets::LiveValueHistory("Velocity", [this]() {
							return glm::length(this->steveJobs.velocity);
						});
						velocityWidget->onDraw += [this](DrawArguments & args) {
							if (this->steveJobs.velocityOK) {
								ofPushStyle();
								ofFill();
								ofSetColor(100, 200, 100);
								ofDrawCircle(args.localBounds.width - 10, 10, 8.0f);
								ofPopStyle();
							}
						};
						inspector->add(velocityWidget);
					}

					inspector->add(new Widgets::Title("Acceleration", Widgets::Title::Level::H3));
					{
						inspector->add(new Widgets::Slider(this->steveJobs.maximumAcceleration));
						auto accelerationWidget = new Widgets::LiveValueHistory("Acceleration", [this]() {
							return glm::length(this->steveJobs.acceleration);
						});
						accelerationWidget->onDraw += [this](DrawArguments & args) {
							if (this->steveJobs.accelerationOK) {
								ofPushStyle();
								ofFill();
								ofSetColor(100, 200, 100);
								ofDrawCircle(args.localBounds.width - 10, 10, 8.0f);
								ofPopStyle();
							}
						};
						inspector->add(accelerationWidget);
					}
				}
			}

			//----------
			glm::vec3 AimMovingHeadAt::getObjectPositionOffset() const {
				return {
					this->objectPositionOffset[0]
					, this->objectPositionOffset[1]
					, this->objectPositionOffset[2]
				};
			}

			//----------
			void AimMovingHeadAt::setObjectPositionOffset(const glm::vec3 & objectPositionOffset) {
				for (int i = 0; i < 3; i++) {
					this->objectPositionOffset[i] = objectPositionOffset[i];
				}
			}

			//----------
			string AimMovingHeadAt::getTargetName() {
				auto target = this->getInput<Item::RigidBody>("Target");
				if (target) return target->getName();
				else return "";
			}

			//----------
			glm::vec3 AimMovingHeadAt::doASteveJobs(const glm::vec3 & targetPosition) {
				//we presume a constant frame rate (necessary for Kalman filter)

				auto dt = ofGetLastFrameTime();

				if (!this->steveJobs.isTracking) {
					//initialise the tracking
					this->initPrediction(targetPosition);
					this->steveJobs.isTracking = true;
					this->steveJobs.position = targetPosition;
				}
				else {
					//update the velocity and acceleration
					auto newVelocity = (targetPosition - this->steveJobs.position) / dt;
					auto newAcceleration = ((newVelocity - this->steveJobs.velocity) / dt) / ofGetFrameRate();
					this->steveJobs.acceleration = this->steveJobs.acceleration * 0.99f + newAcceleration * 0.01f;
					this->steveJobs.velocity = this->steveJobs.velocity * 0.99f + newVelocity * 0.01f;
					this->steveJobs.position = targetPosition;
				}



				//--
				//normal kalman operation
				//--
				//
				auto prediction = this->steveJobs.kalmanFilter.predict();
				for (int i = 0; i < 3; i++) {
					this->steveJobs.measurmentVector.at<float>(i) = targetPosition[i];
				}
				auto corrected = this->steveJobs.kalmanFilter.correct(this->steveJobs.measurmentVector);
				//
				//--



				//--
				//if we dont' meet the thresholds, just pass through the raw tracking data
				//--
				//
				this->steveJobs.velocityOK = glm::length(this->steveJobs.velocity) > this->steveJobs.minimumVelocity;
				this->steveJobs.accelerationOK = glm::length(this->steveJobs.acceleration) < this->steveJobs.maximumAcceleration;
				if(!this->steveJobs.velocityOK && this->steveJobs.accelerationOK) {
					return targetPosition;
				}
				//
				//--



				//--
				//calculate the future after 30 frames
				//--
				//
				auto futureKalman = KalmanFilter(4, 2, 0);
#define COPY_ATTRIB(TARGET, SOURCE, ATTRIBUTE) TARGET.ATTRIBUTE = SOURCE.ATTRIBUTE.clone()
				COPY_ATTRIB(futureKalman, this->steveJobs.kalmanFilter, errorCovPost);
				COPY_ATTRIB(futureKalman, this->steveJobs.kalmanFilter, errorCovPre);
				COPY_ATTRIB(futureKalman, this->steveJobs.kalmanFilter, gain);
				COPY_ATTRIB(futureKalman, this->steveJobs.kalmanFilter, measurementMatrix);
				COPY_ATTRIB(futureKalman, this->steveJobs.kalmanFilter, measurementNoiseCov);
				COPY_ATTRIB(futureKalman, this->steveJobs.kalmanFilter, processNoiseCov);
				COPY_ATTRIB(futureKalman, this->steveJobs.kalmanFilter, statePost);
				COPY_ATTRIB(futureKalman, this->steveJobs.kalmanFilter, statePre);
				COPY_ATTRIB(futureKalman, this->steveJobs.kalmanFilter, transitionMatrix);
				COPY_ATTRIB(futureKalman, this->steveJobs.kalmanFilter, temp1);
				COPY_ATTRIB(futureKalman, this->steveJobs.kalmanFilter, temp2);
				COPY_ATTRIB(futureKalman, this->steveJobs.kalmanFilter, temp3);
				COPY_ATTRIB(futureKalman, this->steveJobs.kalmanFilter, temp4);
				COPY_ATTRIB(futureKalman, this->steveJobs.kalmanFilter, temp5);
				for (int i = 0; i < this->steveJobs.steps; i++) {
					futureKalman.statePost = futureKalman.predict();
				}
				auto theNextIPad = futureKalman.predict();
				//
				//--



				//copy the future predicted state back into the measurement position we'll use
				glm::vec3 futurePosition;
				for (int i = 0; i < 3; i++) {
					futurePosition[i] = theNextIPad.at<float>(i);
				}
				return futurePosition;
			}

			//----------
			void AimMovingHeadAt::initPrediction(const glm::vec3 & startPostiion) {
				//state
				//x, y, vx, vy

				//transition
				this->steveJobs.kalmanFilter.transitionMatrix = Mat_<float>(6, 6) <<
					1, 0, 0, 1, 0, 0,
					0, 1, 0, 0, 1, 0,
					0, 0, 1, 0, 0, 1,
					0, 0, 0, 1, 0, 0,
					0, 0, 0, 0, 1, 0,
					0, 0, 0, 0, 0, 1;
				
				//opening state (presume no velocity to begin)
				this->steveJobs.kalmanFilter.statePre.at<float>(0) = startPostiion.x;
				this->steveJobs.kalmanFilter.statePre.at<float>(1) = startPostiion.y;
				this->steveJobs.kalmanFilter.statePre.at<float>(2) = startPostiion.z;
				this->steveJobs.kalmanFilter.statePre.at<float>(3) = 0.0f;
				this->steveJobs.kalmanFilter.statePre.at<float>(4) = 0.0f;
				this->steveJobs.kalmanFilter.statePre.at<float>(5) = 0.0f;

				setIdentity(this->steveJobs.kalmanFilter.measurementMatrix);
				setIdentity(this->steveJobs.kalmanFilter.processNoiseCov, Scalar::all(1e-4));
				setIdentity(this->steveJobs.kalmanFilter.measurementNoiseCov, Scalar::all(1));
				setIdentity(this->steveJobs.kalmanFilter.errorCovPre, Scalar::all(0.1));

				this->steveJobs.isTracking = true;
				this->steveJobs.position = glm::vec3();
				this->steveJobs.velocity = glm::vec3();
				this->steveJobs.acceleration = glm::vec3();
			}
		}
	}
}
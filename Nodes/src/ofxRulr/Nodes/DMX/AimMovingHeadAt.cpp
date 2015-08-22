#include "AimMovingHeadAt.h"

#include "ofxRulr/Nodes/Item/RigidBody.h"
#include "ofxRulr/Nodes/DMX/MovingHead.h"

#include "ofxCvGui/Widgets/Toggle.h"
#include "ofxCvGui/Widgets/Slider.h"
#include "ofxCvGui/Widgets/Title.h"

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

				//prediction
				{
					this->steveJobs.enabled.set("Enabled", false);
					this->steveJobs.steps.set("Steps", 10, 0, 1000);
					this->steveJobs.kalmanFilter = KalmanFilter(6, 3, 0);
					this->steveJobs.measurmentVector = Mat_<float>(3, 1);
					this->steveJobs.objectSeen = false;
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
						bool doIt = true;
						ofVec3f targetPosition = target->getPosition();

						//check if transform is blank before using it (blank can be an indicator of bad tracking)
						if (this->ignoreBlankTransform) {
							if (target->getTransform().isIdentity()) {
								doIt = false;
								this->steveJobs.objectSeen = false;
							}
						}

						//steve jobs mode
						if (this->steveJobs.enabled) {
							//we presume a constant frame rate (necessary for Kalman filter)

							//if it's the first time to see the object again
							if (!this->steveJobs.objectSeen) {
								//initialise the tracking
								this->initPrediction(targetPosition);
								this->steveJobs.objectSeen = true;
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
							for (int i = 0; i < 3; i++) {
								targetPosition[i] = theNextIPad.at<float>(i);
							}
						}
						//perform the move
						if (doIt) {
							movingHead->lookAt(targetPosition);
						}
					}
					RULR_CATCH_ALL_TO_ERROR;
				}
				else {
					this->steveJobs.objectSeen = false;
				}
			}

			//----------
			void AimMovingHeadAt::serialize(Json::Value & json) {
				Utils::Serializable::serialize(this->ignoreBlankTransform, json);

				auto & jsonPrediction = json["prediction"];
				{
					Utils::Serializable::serialize(this->steveJobs.enabled, json["prediction"]);
					Utils::Serializable::serialize(this->steveJobs.steps, json["prediction"]);
				}
			}

			//----------
			void AimMovingHeadAt::deserialize(const Json::Value & json) {
				Utils::Serializable::deserialize(this->ignoreBlankTransform, json);
				const auto & jsonPrediction = json["prediction"];
				{
					Utils::Serializable::deserialize(this->steveJobs.enabled, json["prediction"]);
					Utils::Serializable::deserialize(this->steveJobs.steps, json["prediction"]);
				}
			}

			//----------
			void AimMovingHeadAt::populateInspector(ofxCvGui::ElementGroupPtr inspector) {
				inspector->add(Widgets::Toggle::make(this->ignoreBlankTransform));

				inspector->add(Widgets::Title::make("Steve Jobs Mode", Widgets::Title::Level::H2));
				{
					inspector->add(Widgets::Title::make("Prediction", Widgets::Title::Level::H3));
					inspector->add(Widgets::Toggle::make(this->steveJobs.enabled));
					inspector->add(Widgets::Slider::make(this->steveJobs.steps));
				}
			}

			//----------
			void AimMovingHeadAt::initPrediction(const ofVec3f & startPostiion) {
				//state
				//x, y, vx, vy

				//transition
				this->steveJobs.kalmanFilter.transitionMatrix = *(Mat_<float>(6, 6) <<
					1, 0, 0, 1, 0, 0,
					0, 1, 0, 0, 1, 0,
					0, 0, 1, 0, 0, 1,
					0, 0, 0, 1, 0, 0,
					0, 0, 0, 0, 1, 0,
					0, 0, 0, 0, 0, 1);
				
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
			}
		}
	}
}
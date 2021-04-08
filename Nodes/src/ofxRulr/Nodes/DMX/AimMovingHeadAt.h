#pragma once

#include "ofxRulr/Nodes/Base.h"
#include "ofxCvMin.h"

namespace ofxRulr {
	namespace Nodes {
		namespace DMX {
			class AimMovingHeadAt : public Nodes::Base {
			public:
				AimMovingHeadAt();
				void init();
				string getTypeName() const override;
				void update();

				void serialize(nlohmann::json &);
				void deserialize(const nlohmann::json &);
				void populateInspector(ofxCvGui::InspectArguments &);

				//A position offset within the objects coordinates
				//We will apply the object's rotation to this
				glm::vec3 getObjectPositionOffset() const;
				void setObjectPositionOffset(const glm::vec3 &);

				string getTargetName();

			protected:
				glm::vec3 doASteveJobs(const glm::vec3 & targetPosition);
				void initPrediction(const glm::vec3 & startPosition);

				ofParameter<bool> ignoreBlankTransform;
				ofParameter<float> objectPositionOffset[3];

				struct {
					ofParameter<bool> enabled;
					ofParameter<float> steps;
					ofParameter<float> minimumVelocity;
					ofParameter<float> maximumAcceleration;

					bool isTracking;
					glm::vec3 position;
					glm::vec3 velocity;
					glm::vec3 acceleration;
					bool velocityOK;
					bool accelerationOK;

					cv::KalmanFilter kalmanFilter;
					cv::Mat measurmentVector;
				} steveJobs;
			};
		}
	}
}
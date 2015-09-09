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

				void serialize(Json::Value &);
				void deserialize(const Json::Value &);
				void populateInspector(ofxCvGui::ElementGroupPtr);

				//A position offset within the objects coordinates
				//We will apply the object's rotation to this
				const ofVec3f & getObjectPositionOffset() const;
				void setObjectPositionOffset(const ofVec3f &);

				string getTargetName();

			protected:
				ofVec3f doASteveJobs(const ofVec3f & targetPosition);
				void initPrediction(const ofVec3f & startPosition);

				ofParameter<bool> ignoreBlankTransform;
				ofParameter<float> objectPositionOffset[3];

				struct {
					ofParameter<bool> enabled;
					ofParameter<float> steps;
					ofParameter<float> minimumVelocity;
					ofParameter<float> maximumAcceleration;

					bool isTracking;
					ofQuaternion rotation;
					ofVec3f position;
					ofVec3f velocity;
					ofVec3f acceleration;
					bool velocityOK;
					bool accelerationOK;

					cv::KalmanFilter kalmanFilter;
					cv::Mat measurmentVector;
				} steveJobs;
			};
		}
	}
}
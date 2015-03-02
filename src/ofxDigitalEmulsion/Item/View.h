#pragma once

#include "RigidBody.h"

#include "../../../addons/ofxRay/src/ofxRay.h"

#define OFXDIGITALEMULSION_VIEW_DISTORTION_COEFFICIENT_COUNT 4

namespace ofxDigitalEmulsion {
	namespace Item {
		class View : public RigidBody {
		public:
			View();
			virtual string getTypeName() const override;

			void init();
			void update();
			void drawObject() override;

			void setDistortionEnabled(bool);
			bool getDistortionEnabled() const;
			virtual float getWidth() const;
			virtual float getHeight() const;

			void setIntrinsics(cv::Mat cameraMatrix, cv::Mat distortionCoefficients = cv::Mat::zeros(OFXDIGITALEMULSION_VIEW_DISTORTION_COEFFICIENT_COUNT, 1, CV_64F));

			cv::Size getSize() const;
			cv::Mat getCameraMatrix() const;
			cv::Mat getDistortionCoefficients() const;

			const ofxRay::Camera & getViewInObjectSpace() const;
			const ofxRay::Camera & getViewInWorldSpace() const;
		protected:
			void rebuildViewFromParameters();
			void rebuildParametersFromView();

			void exportViewMatrix();

			ofParameter<float> focalLengthX, focalLengthY;
			ofParameter<float> principalPointX, principalPointY;

			ofParameter<bool> hasDistortion;
			ofParameter<float> distortion[OFXDIGITALEMULSION_VIEW_DISTORTION_COEFFICIENT_COUNT];

			//Versions of this view as an ofxRay::Camera in world space and object space
			ofxRay::Camera viewInWorldSpace;
			ofxRay::Camera viewInObjectSpace;
		private:
			void serialize(Json::Value &);
			void deserialize(const Json::Value &);
			void populateInspector(ofxCvGui::ElementGroupPtr);
		};
	}
}
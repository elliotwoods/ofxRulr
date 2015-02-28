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

			virtual float getWidth() const;
			virtual float getHeight() const;

			void setIntrinsics(cv::Mat cameraMatrix, cv::Mat distortionCoefficients);
			void setExtrinsics(cv::Mat rotation, cv::Mat translation);

			cv::Mat getCameraMatrix() const;
			cv::Mat getDistortionCoefficients() const;

			const ofxRay::Camera & getRayCameraWorld() const;
			const ofxRay::Camera & getRayCameraObject() const;

			virtual void drawNearClip() { }
		protected:
			ofParameter<float> focalLengthX, focalLengthY;
			ofParameter<float> principalPointX, principalPointY;

			ofParameter<bool> hasDistortion;
			ofParameter<float> distortion[OFXDIGITALEMULSION_VIEW_DISTORTION_COEFFICIENT_COUNT];

			ofxRay::Camera rayCameraWorld;
			ofxRay::Camera rayCameraObject;
		private:
			void serialize(Json::Value &);
			void deserialize(const Json::Value &);
			void populateInspector(ofxCvGui::ElementGroupPtr);
		};
	}
}
#pragma once

#include "Base.h"

#include "ofxCvMin.h"
#include "ofxRay.h"

namespace ofxDigitalEmulsion {
	namespace Item {
		class Projector : public Base {
		public:
			Projector();
			void init();
			string getTypeName() const;

			void serialize(Json::Value &);
			void deserialize(const Json::Value &);

			float getWidth() const;
			float getHeight() const;
			void setWidth(float);
			void setHeight(float);

			void setIntrinsics(cv::Mat cameraMatrix);
			void setExtrinsics(cv::Mat rotation, cv::Mat translation);

			cv::Mat getCameraMatrix() const;

			const ofxRay::Projector & getRayProjector() const;
			void drawWorld() override;

			float getResolutionAspectRatio() const;
			float getPixelAspectRatio() const;

			float getThrowRatioX() const;
			float getThrowRatioY() const;
		protected:
			void populateInspector(ofxCvGui::ElementGroupPtr);

			void rebuildProjector();
			void projectorParameterCallback(float &);
			
			ofxRay::Projector projector;

			ofParameter<float> resolutionWidth, resolutionHeight;
			ofParameter<float> throwRatioX, pixelAspectRatio;
			ofParameter<float> lensOffsetX, lensOffsetY;

			ofParameter<float> translationX, translationY, translationZ;
			ofParameter<float> rotationX, rotationY, rotationZ;
		};
	}
}
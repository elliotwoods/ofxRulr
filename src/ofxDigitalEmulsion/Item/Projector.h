#pragma once

#include "Base.h"

#include "ofxCvMin.h"
#include "ofxRay.h"

namespace ofxDigitalEmulsion {
	namespace Item {
		class Projector : public Base {
		public:
			Projector();
			string getTypeName() const;
			ofxCvGui::PanelPtr getView() override;

			void serialize(Json::Value &) override;
			void deserialize(const Json::Value &) override;

			float getWidth();
			float getHeight();

			void setIntrinsics(cv::Mat cameraMatrix);
			void setExtrinsics(cv::Mat rotation, cv::Mat translation);

			cv::Mat getCameraMatrix() const;

			const ofxRay::Projector & getRayProjector() const;
			void drawWorld();
		protected:
			void populateInspector2(ofxCvGui::ElementGroupPtr);

			void rebuildProjector();
			void projectorParameterCallback(float &);
			
			ofxCvGui::PanelPtr view;

			ofxRay::Projector projector;

			ofParameter<float> resolutionWidth, resolutionHeight;
			ofParameter<float> throwRatioX, throwRatioY;
			ofParameter<float> lensOffsetX, lensOffsetY;

			ofParameter<float> translationX, translationY, translationZ;
			ofParameter<float> rotationX, rotationY, rotationZ;
		};
	}
}
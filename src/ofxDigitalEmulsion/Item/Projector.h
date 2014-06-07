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

			void drawWorld();
		protected:
			void populateInspector2(ofxCvGui::ElementGroupPtr);

			ofxRay::Projector projector;
		};
	}
}
#pragma once

#include "Base.h"
#include "ofxCvMin.h"

namespace ofxDigitalEmulsion {
	namespace Item {
		class Checkerboard : public Base {
		public:
			Checkerboard();
			void init();
			string getTypeName() const override;
			ofxCvGui::PanelPtr getView();

			void serialize(Json::Value &) override;
			void deserialize(const Json::Value &) override;

			cv::Size getSize() const;
			vector<cv::Point3f> getObjectPoints() const;
		protected:
			void populateInspector2(ofxCvGui::ElementGroupPtr) override;
			void updatePreviewMesh();

			ofParameter<float> sizeX, sizeY;
			ofParameter<float> spacing;
			ofMesh previewMesh;
		};
	}
}
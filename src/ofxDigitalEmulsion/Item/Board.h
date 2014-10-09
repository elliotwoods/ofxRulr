#pragma once

#include "Base.h"
#include "ofxCvMin.h"

namespace ofxDigitalEmulsion {
	namespace Item {
		class Board : public Base {
		public:
			Board();
			string getTypeName() const override;
			ofxCvGui::PanelPtr getView();

			void serialize(Json::Value &) override;
			void deserialize(const Json::Value &) override;

			ofxCv::BoardType getBoardType() const;
			cv::Size getSize() const;
			vector<cv::Point3f> getObjectPoints() const;

			bool findBoard(cv::Mat, vector<cv::Point2f> & result) const;
		protected:
			void populateInspector2(ofxCvGui::ElementGroupPtr) override;
			void updatePreviewMesh();

			ofParameter<int> boardType;
			ofParameter<float> sizeX, sizeY;
			ofParameter<float> spacing;
			ofMesh previewMesh;
		};
	}
}
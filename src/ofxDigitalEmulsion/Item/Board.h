#pragma once

#include "Base.h"
#include "ofxCvMin.h"

namespace ofxDigitalEmulsion {
	namespace Item {
		class Board : public Base {
		public:
			Board();
			void init();
			string getTypeName() const override;
			ofxCvGui::PanelPtr getView();

			void serialize(Json::Value &);
			void deserialize(const Json::Value &);

			ofxCv::BoardType getBoardType() const;
			cv::Size getSize() const;
			vector<cv::Point3f> getObjectPoints() const;

			bool findBoard(cv::Mat, vector<cv::Point2f> & result, bool useOptimisers = true) const;
		protected:
			void populateInspector(ofxCvGui::ElementGroupPtr);
			void updatePreviewMesh();

			ofParameter<int> boardType; // 0 = checkerboard, 1 = circles
			ofParameter<float> sizeX, sizeY;
			ofParameter<float> spacing;
			ofMesh previewMesh;
		};
	}
}
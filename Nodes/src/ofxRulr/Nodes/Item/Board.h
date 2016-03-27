#pragma once

#include "Base.h"
#include "ofxCvMin.h"

namespace ofxRulr {
	namespace Nodes {
		namespace Item {
			class Board : public Base {
			public:
				Board();
				void init();
				string getTypeName() const override;
				ofxCvGui::PanelPtr getPanel() override;

				void serialize(Json::Value &);
				void deserialize(const Json::Value &);

				ofxCv::BoardType getBoardType() const;
				cv::Size getSize() const;
				float getSpacing() const;
				vector<cv::Point3f> getObjectPoints() const;
				void drawObject() const;

				bool findBoard(cv::Mat, vector<cv::Point2f> & result, bool useOptimisers = true) const;
			protected:
				void populateInspector(ofxCvGui::InspectArguments &);
				void updatePreviewMesh();

				ofxCvGui::PanelPtr view;
				ofParameter<int> boardType; // 0 = checkerboard, 1 = circles
				ofParameter<float> sizeX, sizeY;
				ofParameter<float> spacing;
				ofMesh previewMesh;
			};
		}
	}
}
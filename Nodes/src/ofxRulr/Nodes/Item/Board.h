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
				ofMesh previewMesh;

				struct : ofParameterGroup {
					ofParameter<int> boardType{ "Board type", 0, 0, 1 }; // 0 = checkerboard, 1 = circles
					ofParameter<float> sizeX{ "Size X", 10.0f, 2.0f, 20.0f };
					ofParameter<float> sizeY{ "Size Y", 7.0f, 2.0f, 20.0f };
					ofParameter<float> spacing{ "Spacing [m]", 0.026f, 0.001f, 1.0f };
					PARAM_DECLARE("Board", boardType, sizeX, sizeY, spacing);
				} parameters;
			};
		}
	}
}
#pragma once

#include "AbstractBoard.h"
#include "ofxCvMin.h"

namespace ofxRulr {
	namespace Nodes {
		namespace Item {
			class Board : public AbstractBoard {
			public:
				Board();
				void init();
				string getTypeName() const override;
				ofxCvGui::PanelPtr getPanel() override;

				void serialize(Json::Value &);
				void deserialize(const Json::Value &);

				ofxCv::BoardType getBoardType() const;
				cv::Size getSize() const;
				vector<cv::Point3f> getObjectPoints() const;
				void drawObject() const override;
				float getSpacing() const override;

				bool findBoard(cv::Mat, vector<cv::Point2f> & result, vector<cv::Point3f> & objectPoints, FindBoardMode findBoardMode, cv::Mat cameraMatrix, cv::Mat distortionCoefficients) const override;
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

					struct : ofParameterGroup {
						ofParameter<bool> centered{ "Centered", true };
						ofParameter<float> x{ "X", 0, -1.0f, 1.0f };
						ofParameter<float> y{ "Y", 0, -1.0f, 1.0f };
						ofParameter<float> z{ "Z", 0, -1.0f, 1.0f };
						PARAM_DECLARE("Offset", x, y, z);
					} offset;

					PARAM_DECLARE("Board", boardType, sizeX, sizeY, spacing, offset);
				} parameters;
			};
		}
	}
}
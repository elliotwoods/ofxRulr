#pragma once

#include "ofxRulr/Nodes/Item/AbstractBoard.h"
#include <opencv2/aruco/dictionary.hpp>
#include <opencv2/aruco/charuco.hpp>

namespace ofxRulr {
	namespace Nodes {
		namespace ArUco {
			class ChArUcoBoard : public Nodes::Item::AbstractBoard {
			public:
				ChArUcoBoard();
				string getTypeName() const override;
				void init();
				void update();
				void serialize(Json::Value &);
				void deserialize(const Json::Value &);
				void populateInspector(ofxCvGui::InspectArguments &);

				bool findBoard(cv::Mat, vector<cv::Point2f> & imagePoints, vector<cv::Point3f> & objectPoints, FindBoardMode findBoardMode, cv::Mat cameraMatrix, cv::Mat distortionCoefficients) const override;
				ofxCvGui::PanelPtr getPanel() override;

			protected:
				struct : ofParameterGroup {
					struct : ofParameterGroup {
						ofParameter<int> width{ "Width", 12 };
						ofParameter<int> height{ "Height", 8 };
						PARAM_DECLARE("Size", width, height);
					} size;

					ofParameter<float> squareLength{ "Square length", 0.034875, 0.001, 0.10 }; // noah's board
					ofParameter<float> markerLength{ "Marker length", 0.0175, 0.001, 1.0 };
					ofParameter<int> previewDPI{ "Preview DPI", 75 };

					struct : ofParameterGroup {
						ofParameter<bool> refineStrategy{ "Refine strategy", true };
						PARAM_DECLARE("Detection", refineStrategy);
					} detection;

					PARAM_DECLARE("ChAruCoBoard", size, squareLength, markerLength, detection);
				} parameters;

				cv::Ptr<cv::aruco::Dictionary> dictionary;
				cv::Ptr<cv::aruco::CharucoBoard> board;
				ofImage preview;
				ofxCvGui::PanelPtr panel;
			};
		}
	}
}
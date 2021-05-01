#pragma once

#include "ofxRulr.h"
#include "ofxCvMin.h"

#include "ofxRulr/Nodes/Item/AbstractBoard.h"

namespace ofxRulr {
	namespace Nodes {
		namespace Procedure {
			namespace Calibrate {
				class CameraExtrinsicsFromBoardInWorld : public Base {
				public:
					CameraExtrinsicsFromBoardInWorld();
					void init();
					string getTypeName() const override;
					ofxCvGui::PanelPtr getPanel() override;
					void update();
					void drawWorldStage();

					void serialize(nlohmann::json&);
					void deserialize(const nlohmann::json&);
					void populateInspector(ofxCvGui::InspectArguments&);

					void captureAndCalibrate();
				protected:
					ofxCvGui::PanelPtr view;

					ofImage image;
					vector<cv::Point2f> imagePoints;
					vector<cv::Point3f> objectPoints;

					struct : ofParameterGroup {
						struct : ofParameterGroup {
							ofParameter<FindBoardMode> findBoardMode{ "Find board mode", FindBoardMode::Optimized };
							PARAM_DECLARE("Capture", findBoardMode);
						} capture;

						PARAM_DECLARE("CameraExtrinsicsFromBoardInWorld", capture);
					} parameters;
				};
			}
		}
	}
}
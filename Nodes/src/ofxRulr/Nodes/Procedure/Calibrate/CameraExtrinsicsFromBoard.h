#pragma once

#include "../Base.h"
#include "ofxCvMin.h"

#include "ofxRulr/Nodes/Item/Board.h"

namespace ofxRulr {
	namespace Nodes {
		namespace Procedure {
			namespace Calibrate {
				class CameraExtrinsicsFromBoard : public Base {
				public:
					CameraExtrinsicsFromBoard();
					void init();
					string getTypeName() const override;
					ofxCvGui::PanelPtr getPanel() override;
					void update();
					void drawWorld();

					void serialize(Json::Value &);
					void deserialize(const Json::Value &);
					void populateInspector(ofxCvGui::InspectArguments &);

					void captureOriginBoard();
					void capturePositiveXBoard();
					void captureXZPlaneBoard();

				protected:
					void captureTo(ofParameter<ofVec3f> &);
					void calibrate();

					ofxCvGui::PanelPtr view;
					ofImage grayscalePreview;

					vector<ofVec2f> currentCorners;
					struct : ofParameterGroup {
						struct : ofParameterGroup {
							ofParameter<bool> useOptimizers{ "Use optimizers", false };
							ofParameter<Item::Board::FindBoardMode> findBoardMode{ "Find board mode", Item::Board::FindBoardMode::Optimized };

							PARAM_DECLARE("Capture", findBoardMode);
						} capture;

						struct : ofParameterGroup {
							ofParameter<ofVec3f> originInCameraObjectSpace{ "Origin", {0, 0, 0} };
							ofParameter<ofVec3f> positiveXInCameraObjectSpace{ "+X", {1, 0, 0 } };
							ofParameter<ofVec3f> positionInXZPlaneInCameraObjectSpace{ "Point in XZ plane", {1, 0, 1} };
							PARAM_DECLARE("Calibration points"
								, originInCameraObjectSpace
								, positiveXInCameraObjectSpace
								, positionInXZPlaneInCameraObjectSpace);
						} calibrationPoints;

						PARAM_DECLARE("CameraExtrinsicsFromBoard", capture, calibrationPoints);
					} parameters;
				};
			}
		}
	}
}
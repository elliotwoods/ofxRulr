#pragma once

#include "ofxRulr.h"
#include "ofxCvMin.h"

#include "ofxRulr/Nodes/Item/AbstractBoard.h"

namespace ofxRulr {
	namespace Nodes {
		namespace Procedure {
			namespace Calibrate {
				class CameraExtrinsicsFromBoardPlanes : public Base {
				public:
					CameraExtrinsicsFromBoardPlanes();
					void init();
					string getTypeName() const override;
					ofxCvGui::PanelPtr getPanel() override;
					void update();
					void drawWorldStage();

					void serialize(nlohmann::json &);
					void deserialize(const nlohmann::json &);
					void populateInspector(ofxCvGui::InspectArguments &);

					void captureOriginBoard();
					void capturePositiveXBoard();
					void captureXZPlaneBoard();

				protected:
					void captureTo(ofParameter<glm::vec3> &);
					void calibrate();

					ofxCvGui::PanelPtr view;
					ofImage grayscalePreview;

					vector<glm::vec2> currentCorners;
					vector<glm::vec3> currentObjectPoints;

					struct : ofParameterGroup {
						struct : ofParameterGroup {
							ofParameter<bool> useOptimizers{ "Use optimizers", false };
							ofParameter<FindBoardMode> findBoardMode{ "Find board mode", FindBoardMode::Optimized };

							PARAM_DECLARE("Capture", findBoardMode);
						} capture;

						struct : ofParameterGroup {
							ofParameter<glm::vec3> originInCameraObjectSpace{ "Origin", {0, 0, 0} };
							ofParameter<glm::vec3> positiveXInCameraObjectSpace{ "+X", {1, 0, 0 } };
							ofParameter<glm::vec3> positionInXZPlaneInCameraObjectSpace{ "Point in XZ plane", {1, 0, 1} };
							PARAM_DECLARE("Calibration points"
								, originInCameraObjectSpace
								, positiveXInCameraObjectSpace
								, positionInXZPlaneInCameraObjectSpace);
						} calibrationPoints;

						PARAM_DECLARE("CameraExtrinsicsFromBoardPlanes", capture, calibrationPoints);
					} parameters;
				};
			}
		}
	}
}
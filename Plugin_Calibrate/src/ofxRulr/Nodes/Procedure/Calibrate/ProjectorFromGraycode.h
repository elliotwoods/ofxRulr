#pragma once

#include "ofxRulr/Nodes/Base.h"
#include "ofxRulr/Nodes/Item/AbstractBoard.h"

#include "ofxRulr/Nodes/Item/AbstractBoard.h"
#include "ofxRulr/Nodes/Item/Projector.h"
#include "ofxRulr/Utils/CaptureSet.h"
#include "Constants_Plugin_Calibration.h"

namespace ofxRulr {
	namespace Nodes {
		namespace Procedure {
			namespace Calibrate {
				class PLUGIN_CALIBRATION_EXPORTS ProjectorFromGraycode : public Base {
				public:
					class Capture : public Utils::AbstractCaptureSet::BaseCapture {
					public:
						Capture();
						string getDisplayString() const override;
						vector<ofVec3f> worldPoints;
						vector<ofVec2f> cameraImagePoints;
						vector<ofVec2f> projectorImagePoints;
						ofMatrix4x4 transform;

						void serialize(Json::Value &);
						void deserialize(const Json::Value &);
						void drawWorld(shared_ptr<Item::Projector>);

						void drawOnCameraImage();
						void drawOnProjectorImage();
					};

					ProjectorFromGraycode();
					string getTypeName() const override;
					ofxCvGui::PanelPtr getPanel() override;

					void init();
					void update();
					void drawWorldStage();
					
					void addCapture();
					void calibrate();

					void populateInspector(ofxCvGui::InspectArguments &);
					void serialize(Json::Value &);
					void deserialize(const Json::Value &);
				protected:
					ofxCvGui::PanelPtr panel;

					struct : ofParameterGroup {
						struct : ofParameterGroup {
							ofParameter<bool> autoScan{ "Automatically scan", true };
							ofParameter<float> brightAreaThreshold{ "Bright area threshold", 64, 0, 255 };
							ofParameter<int> maximumDelauneyPoints{"Maximum delauney points", 100000 };
							ofParameter<FindBoardMode> findBoardMode{ "Find board mode", FindBoardMode::Optimized };
							ofParameter<bool> useRansacForSolvePnp{ "Use RANSAC for SolvePNP", false };
							ofParameter<float> erosion{ "Erosion (/Board size)", 0.02f, 0.0f, 0.1f };
							ofParameter<int> maxTriangleArea{ "Max triangle area", 100*100 };
							PARAM_DECLARE("Capture"
								, autoScan
								, brightAreaThreshold
								, maximumDelauneyPoints
								, findBoardMode
								, useRansacForSolvePnp
								, erosion
								, maxTriangleArea);
						} capture;
						ofParameter<string> selection{ "Selection", "" };
						PARAM_DECLARE("ProjectorFromGraycode", capture, selection);
					} parameters;

					Utils::CaptureSet<Capture> captures;
					float error = 0.0f;

					struct {
						ofTexture projectorInCamera;
						ofTexture cameraInProjector;
					} preview;
				};
			}
		}
	}
}
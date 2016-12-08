#pragma once

#include "ofxRulr/Nodes/Base.h"
#include "ofxRulr/Nodes/Item/Board.h"

namespace ofxRulr {
	namespace Nodes {
		namespace Procedure {
			namespace Calibrate {
				class ProjectorFromGraycode : public Base {
				public:
					struct Capture {
						vector<ofVec3f> worldPoints;
						vector<ofVec2f> projectorImagePoints;
					};

					ProjectorFromGraycode();
					string getTypeName() const override;
					ofxCvGui::PanelPtr getPanel() override;

					void init();
					void drawWorld();
					
					void addCapture();
					void deleteLastCapture();
					void clearCaptures();
					void calibrate();

					void populateInspector(ofxCvGui::InspectArguments &);
					void serialize(Json::Value &);
					void deserialize(const Json::Value &);

					vector<int> getSelection() const;
					void deleteSelection();
				protected:
					ofxCvGui::PanelPtr panel;

					struct : ofParameterGroup {
						struct : ofParameterGroup {
							ofParameter<bool> autoScan{ "Automatically scan", true };
							ofParameter<bool> searchBrightArea{ "Search bright area", true };
							ofParameter<float> brightAreaThreshold{ "Bright area threshold", 64, 0, 255 };
							ofParameter<Item::Board::FindBoardMode> findBoardMode{ "Find board mode", Item::Board::FindBoardMode::Optimized };
							ofParameter<bool> useRansacForSolvePnp{ "Use RANSAC for SolvePNP", true };
							ofParameter<float> erosion{ "Erosion (/Board size)", 0.02f, 0.0f, 0.1f };
							ofParameter<int> maxTriangleArea{ "Max triangle area", 100*100 };
							PARAM_DECLARE("Capture", autoScan, searchBrightArea, brightAreaThreshold, findBoardMode, erosion, maxTriangleArea);
						} capture;
						ofParameter<string> selection{ "Selection", "" };
						PARAM_DECLARE("ProjectorFromGraycode", capture, selection);
					} parameters;

					vector<Capture> captures;
					ofFloatImage preview;
					float error = 0.0f;
				};
			}
		}
	}
}
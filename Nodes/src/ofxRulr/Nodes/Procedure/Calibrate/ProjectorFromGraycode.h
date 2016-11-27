#pragma once

#include "ofxRulr/Nodes/Base.h"

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
					void clearCaptures();
					void calibrate();

					void populateInspector(ofxCvGui::InspectArguments &);
					void serialize(Json::Value &);
					void deserialize(const Json::Value &);
				protected:
					ofxCvGui::PanelPtr panel;

					struct : ofParameterGroup {
						struct : ofParameterGroup {
							ofParameter<bool> autoScan{ "Automatically scan", true };
							ofParameter<bool> searchBrightArea{ "Search bright area", true };
							ofParameter<float> brightAreaThreshold{ "Bright area threshold", 64, 0, 255 };
							ofParameter<bool> useOptimizedImage{ "Use optimized image", false };
							ofParameter<bool> useRansacForSolvePnp{ "Use RANSAC for SolvePNP", true };
							PARAM_DECLARE("Capture", autoScan, searchBrightArea, brightAreaThreshold, useOptimizedImage);
						} capture;
						PARAM_DECLARE("ProjectorFromGraycode", capture);
					} parameters;

					vector<Capture> captures;
					ofFloatImage preview;
					float error = 0.0f;
				};
			}
		}
	}
}
#pragma once

#include "ofxRulr/Nodes/Base.h"
#include "ofxRulr/Utils/CaptureSet.h"
#include "ofxRulr/Utils/VideoOutputListener.h"
#include "ofxRulr/Nodes/Item/Board.h"

namespace ofxRulr {
	namespace Nodes {
		namespace Procedure {
			namespace Calibrate {
				class ProjectorFromStereoAndHelperCamera : public Nodes::Base {
				public:
					class Capture : public Utils::AbstractCaptureSet::BaseCapture {
					public:
						Capture();
						void serialize(Json::Value &);
						void deserialize(const Json::Value &);
						string getDisplayString() const override;

						vector<ofVec3f> worldSpacePoints;
						vector<ofVec2f> imageSpacePoints;
						vector<ofVec2f> reprojectedImageSpacePoints;
					};

					ProjectorFromStereoAndHelperCamera();
					string getTypeName() const override;

					void init();
					void update();
					
					void serialize(Json::Value &);
					void deserialize(const Json::Value &);
					void populateInspector(ofxCvGui::InspectArguments &);

					ofxCvGui::PanelPtr getPanel() override;

					void addCapture();
					void calibrate();
				protected:
					void drawWorld();
					void drawOnOutput(const ofRectangle & bounds);

					Utils::CaptureSet<Capture> captures;
					ofxCvGui::PanelPtr panel;

					struct : ofParameterGroup {
						struct : ofParameterGroup {
							ofParameter<bool> useExistingGraycodeScan{ "Use existing graycode scan", false };
							ofParameter<Item::AbstractBoard::FindBoardMode> stereoFindBoardMode{ "Stereo find board mode", Item::AbstractBoard::FindBoardMode::Optimized };
							ofParameter<Item::AbstractBoard::FindBoardMode> helperFindBoardMode{ "Helper find board mode", Item::AbstractBoard::FindBoardMode::Assistant };
							ofParameter<float> helperPixelsSeachDistance{ "Helper pixels search distance", 3, 0, 10 };
							ofParameter<bool> correctStereoMatches{ "Correct stereo matches", false };
							PARAM_DECLARE("Capture", useExistingGraycodeScan, stereoFindBoardMode, helperFindBoardMode, helperPixelsSeachDistance, correctStereoMatches);
						} capture;

						struct : ofParameterGroup {
							ofParameter<float> initialLensOffset{ "Initial lens offset", 0, -1.0, 1.0 };
							ofParameter<float> initialThrowRatio{ "Initial throw ratio", 1, 0, 5 };
							ofParameter<bool> useDecimation{ "Use decimation", false };
							struct : ofParameterGroup {
								ofParameter<bool> enabled{ "Enabled", false };
								ofParameter<float> maxReprojectionError{ "Max reprojection error [px]", 10.0f };
								PARAM_DECLARE("Remove outliers", enabled, maxReprojectionError);
							} removeOutliers;

							PARAM_DECLARE("Calibrate", initialLensOffset, initialThrowRatio, useDecimation, removeOutliers);
						} calibrate;

						struct : ofParameterGroup {
							ofParameter<bool> dataInWorld{ "Data in world", true };
							ofParameter<bool> dataOnVideoOutput{ "Data on VideoOutput", true };
							ofParameter<bool> reprojectedOnVideoOutput{ "Reprojected points on VideoOutput", true };

							PARAM_DECLARE("Draw", dataInWorld, dataOnVideoOutput, reprojectedOnVideoOutput)
						} draw;
						PARAM_DECLARE("ProjectorFromStereoAndHelperCamera", capture, calibrate, draw);
					} parameters;

					ofParameter<float> reprojectionError{ "Reprojection error [px]", 0 };
					unique_ptr<Utils::VideoOutputListener> videoOutputListener;
				};
			}
		}
	}
}
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
						ofParameter<bool> drawBoards{ "Draw boards", true };
						struct : ofParameterGroup {
							ofParameter<Item::Board::FindBoardMode> stereoFindBoardMode{ "Stereo find board mode", Item::Board::FindBoardMode::Optimized };
							ofParameter<Item::Board::FindBoardMode> helperFindBoardMode{ "Helper find board mode", Item::Board::FindBoardMode::Optimized };
							ofParameter<float> helperPixelsSeachDistance{ "Helper pixels search distance", 3, 0, 10 };
							ofParameter<bool> correctStereoMatches{ "Correct stereo matches", false };
							PARAM_DECLARE("Capture", stereoFindBoardMode, helperFindBoardMode, helperPixelsSeachDistance, correctStereoMatches);
						} capture;
						PARAM_DECLARE("CameraIntrinsics", drawBoards, capture);
					} parameters;

					unique_ptr<Utils::VideoOutputListener> videoOutputListener;
				};
			}
		}
	}
}
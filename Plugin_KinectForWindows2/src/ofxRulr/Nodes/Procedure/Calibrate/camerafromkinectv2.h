#pragma once

#include "ofxRulr/Nodes/Procedure/Base.h"
#include "ofxCvGui/Panels/Groups/Grid.h"

#include "ofxRulr/Nodes/Item/Board.h"

namespace ofxRulr {
	namespace Nodes {
		namespace Procedure {
			namespace Calibrate {
				class CameraFromKinectV2 : public ofxRulr::Nodes::Procedure::Base {
				public:
					struct Correspondence {
						ofVec3f kinectObject;
						ofVec2f camera;
						ofVec2f cameraNormalized;
					};

					CameraFromKinectV2();
					string getTypeName() const override;
					void init();
					ofxCvGui::PanelPtr getPanel() override;
					void update();

					void serialize(Json::Value &);
					void deserialize(const Json::Value &);

					void addCapture();
					void calibrate();
				protected:
					void populateInspector(ofxCvGui::InspectArguments &);
					void drawWorld();
					void rebuildView();
					shared_ptr<ofxCvGui::Panels::Groups::Grid> view;

					struct : ofParameterGroup {
						ofParameter<Item::Board::FindBoardMode> findBoardMode{ "Mode", Item::Board::FindBoardMode::Raw };
						PARAM_DECLARE("CameraFromKinectV2", findBoardMode);
					} parameters;

					vector<Correspondence> correspondences;
					vector<ofVec2f> previewCornerFindsKinect;
					vector<ofVec2f> previewCornerFindsCamera;
					float error;

					chrono::system_clock::time_point lastTimeCheckerboardSeenInKinect;
					chrono::system_clock::time_point lastTimeCheckerboardSeenInCamera;
				};
			}
		}
	}
}
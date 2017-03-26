#pragma once

#include "../Base.h"
#include "ofxCvMin.h"
#include "ofxRulr/Nodes/Item/Board.h"
#include "ofxRulr/Utils/CaptureSet.h"

namespace ofxRulr {
	namespace Nodes {
		namespace Procedure {
			namespace Calibrate {
				class CameraIntrinsics : public Base {
				public:
					class Capture : public Utils::AbstractCaptureSet::BaseCapture {
					public:
						Capture();

						string getDisplayString() const override;
						void drawWorld();
						void drawOnImage() const;

						vector<ofVec2f> pointsImageSpace;
						vector<ofVec3f> pointsObjectSpace;

						ofParameter<float> imageWidth{ "Image width", 0.0f };
						ofParameter<float> imageHeight{ "Image height", 0.0f };

						ofParameter<ofMatrix4x4> extrsinsics{ "Extrinsics", ofMatrix4x4() };
						ofParameter<float> reprojectionError{ "Reprojection error", 0.0f };
					protected:
						void serialize(Json::Value &);
						void deserialize(const Json::Value &);
					};

					CameraIntrinsics();
					void init();
					string getTypeName() const override;
					ofxCvGui::PanelPtr getPanel() override;

					void update();
					void drawWorld();

					void serialize(Json::Value &);
					void deserialize(const Json::Value &);

					bool getRunFinderEnabled() const;
				protected:
					void populateInspector(ofxCvGui::InspectArguments &);
					void addBoard(bool tetheredCapture);
					void findBoard();
					void calibrate();

					shared_ptr<ofxCvGui::Panels::BaseImage> view;
					ofImage grayscale;

					Utils::CaptureSet<Capture> captures;

					vector<ofVec2f> currentCorners;
					ofParameter<float> error{ "Reprojection error [px]", 0.0f };

					struct : ofParameterGroup {
						ofParameter<bool> drawBoards{ "Draw boards", true };
						struct : ofParameterGroup {
							ofParameter<bool> tetheredShootMode{ "Tethered shoot mode", true };
							ofParameter<Item::Board::FindBoardMode> findBoardMode{ "Mode", Item::Board::FindBoardMode::Optimized };

							PARAM_DECLARE("Capture", tetheredShootMode, findBoardMode);
						} capture;
						PARAM_DECLARE("CameraIntrinsics", drawBoards, capture);
					} parameters;
					
				};
			}
		}
	}
}
#pragma once

#include "ofxRulr/Nodes/Procedure/Base.h"
#include "ofxCvGui/Panels/Groups/Grid.h"

#include "ofxRulr/Nodes/Item/AbstractBoard.h"
#include "ofxRulr/Utils/CaptureSet.h"

namespace ofxRulr {
	namespace Nodes {
		namespace Procedure {
			namespace Calibrate {
				class CameraFromKinectV2 : public ofxRulr::Nodes::Procedure::Base {
				public:
					class Capture : public Utils::AbstractCaptureSet::BaseCapture {
					public:
						Capture();
						string getDisplayString() const override;
						void serialize(nlohmann::json &);
						void deserialize(const nlohmann::json &);
						void drawObjectSpace();
						void drawKinectImageSpace();

						vector<cv::Point2f> kinectImageSpace;
						vector<cv::Point3f> kinectObjectSpace;
						vector<cv::Point2f> cameraImageSpace;
						vector<cv::Point2f> cameraNormalizedSpace;
					};

					CameraFromKinectV2();
					string getTypeName() const override;
					void init();
					ofxCvGui::PanelPtr getPanel() override;
					void update();

					void serialize(nlohmann::json &);
					void deserialize(const nlohmann::json &);

					void addCapture();
					void calibrate();
				protected:
					void populateInspector(ofxCvGui::InspectArguments &);
					void drawWorldStage();
					void rebuildView();
					shared_ptr<ofxCvGui::Panels::Groups::Grid> view;

					struct : ofParameterGroup {
						ofParameter<FindBoardMode> findBoardMode{ "Mode", FindBoardMode::Raw };
						PARAM_DECLARE("CameraFromKinectV2", findBoardMode);
					} parameters;

					Utils::CaptureSet<Capture> captures;

					float error = 0.0f;

					chrono::system_clock::time_point lastTimeCheckerboardSeenInKinect;
					chrono::system_clock::time_point lastTimeCheckerboardSeenInCamera;
				};
			}
		}
	}
}
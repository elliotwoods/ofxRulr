#pragma once

#include "ofxRulr.h"
#include "ofxCvMin.h"
#include "ofxRulr/Nodes/Item/AbstractBoard.h"
#include "ofxRulr/Utils/CaptureSet.h"

#include "Constants_Plugin_Calibration.h"

namespace ofxRulr {
	namespace Nodes {
		namespace Procedure {
			namespace Calibrate {
				class PLUGIN_CALIBRATE_EXPORTS StereoCalibrate : public Base {
				public:
					MAKE_ENUM(PreviewStyle
						, (Live, LastCapture)
						, ("Live", "Last Capture"));

					class Capture : public Utils::AbstractCaptureSet::BaseCapture {
					public:
						Capture();

						string getDisplayString() const override;
						void drawOnImageA() const;
						void drawOnImageB() const;

						vector<ofVec3f> pointsObjectSpace;
						vector<ofVec2f> pointsImageSpaceA;
						vector<ofVec2f> pointsImageSpaceB;

						vector<ofVec3f> pointsWorldSpace;
					protected:
						void serialize(Json::Value &);
						void deserialize(const Json::Value &);
					};

					struct OpenCVCalibration {
						cv::Mat rotation3x3;
						cv::Mat rotationVector;
						cv::Mat translation;
						cv::Mat essential;
						cv::Mat fundamental;
						cv::Mat rectificationRotationA;
						cv::Mat rectificationRotationB;
						cv::Mat projectionA;
						cv::Mat projectionB;
						cv::Mat disparityToDepth;
					};

					StereoCalibrate();
					string getTypeName() const override;

					void init();
					ofxCvGui::PanelPtr getPanel() override;

					void update();
					void drawWorldStage();

					void serialize(Json::Value &);
					void deserialize(const Json::Value &);

					const OpenCVCalibration & getOpenCVCalibration() const;
					vector<ofVec3f> triangulate(const vector<ofVec2f> & imagePointsA, const vector<ofVec2f> & imagePointsB, bool correctMatches);

					void throwIfACameraIsDisconnected();
					void updateTransform();
				protected:
					void populateInspector(ofxCvGui::InspectArguments &);
					void addCapture();
					void calibrate();

					ofxCvGui::PanelPtr view;
					ofTexture previewA, previewB;
					ofPixels lastCaptureImageA, lastCaptureImageB;
					bool previewIsLastCapture = true;

					Utils::CaptureSet<Capture> captures;
					OpenCVCalibration openCVCalibration;

					struct : ofParameterGroup {
						ofParameter<PreviewStyle> previewStyle {"Preview", PreviewStyle::Live};

						struct : ofParameterGroup {
							ofParameter<FindBoardMode> findBoardMode{ "Mode", FindBoardMode::Optimized };
							PARAM_DECLARE("Capture", findBoardMode);
						} capture;

						struct : ofParameterGroup {
							ofParameter<bool> fixIntrinsics{ "Fix intrinsics", true };
							PARAM_DECLARE("Calibrate", fixIntrinsics);
						} calibration;

						struct : ofParameterGroup {
							ofParameter<bool> points{ "Points", true };
							ofParameter<bool> distances{ "Distances", false };
							PARAM_DECLARE("Draw", points, distances);
						} draw;

						PARAM_DECLARE("StereoCalibrate", previewStyle, capture, calibration, draw);
					} parameters;

					ofParameter<float> reprojectionError{ "Reprojection error [px]", 0.0f };
					struct {
						chrono::system_clock::time_point lastFailureA = chrono::system_clock::now() - chrono::minutes(1);
						chrono::system_clock::time_point lastFailureB = chrono::system_clock::now() - chrono::minutes(1);
					} lastFailures;
				};
			}
		}
	}
}
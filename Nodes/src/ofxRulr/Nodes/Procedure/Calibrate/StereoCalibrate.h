#pragma once

#include "../Base.h"
#include "ofxCvMin.h"
#include "ofxRulr/Nodes/Item/AbstractBoard.h"
#include "ofxRulr/Utils/CaptureSet.h"

namespace ofxRulr {
	namespace Nodes {
		namespace Procedure {
			namespace Calibrate {
				class StereoCalibrate : public Base {
				public:
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
						cv::Mat rotation;
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
					void drawWorld();

					void serialize(Json::Value &);
					void deserialize(const Json::Value &);

					const OpenCVCalibration & getOpenCVCalibration() const;
					vector<ofVec3f> triangulate(const vector<ofVec2f> & imagePointsA, const vector<ofVec2f> & imagePointsB, bool correctMatches);
					bool solvePnP(const vector<cv::Point2f> & imagePointsA, const vector<cv::Point2f> & imagePointsB, const vector<cv::Point3f> & objectPoints, cv::Mat rotationVector, cv::Mat translation, bool useExtrinsicGuess = true);

					void throwIfACameraIsDisconnected();
				protected:
					void populateInspector(ofxCvGui::InspectArguments &);
					void addCapture();
					void calibrate();

					ofxCvGui::PanelPtr view;
					ofImage previewA, previewB;

					Utils::CaptureSet<Capture> captures;
					OpenCVCalibration openCVCalibration;

					struct : ofParameterGroup {
						struct : ofParameterGroup {
							ofParameter<Item::AbstractBoard::FindBoardMode> findBoardMode{ "Mode", Item::AbstractBoard::FindBoardMode::Optimized };
							PARAM_DECLARE("Capture", findBoardMode);
						} capture;

						struct : ofParameterGroup {
							ofParameter<bool> fixIntrinsics{ "Fix intrinsics", false };
							PARAM_DECLARE("Calibrate", fixIntrinsics);
						} calibration;

						struct : ofParameterGroup {
							ofParameter<bool> enabled{ "Enabled", true };
							ofParameter<bool> points{ "Points", true };
							ofParameter<bool> distances{ "Distances", false };
							PARAM_DECLARE("Draw", enabled, points, distances);
						} draw;

						PARAM_DECLARE("StereoCalibrate", capture, calibration, draw);
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
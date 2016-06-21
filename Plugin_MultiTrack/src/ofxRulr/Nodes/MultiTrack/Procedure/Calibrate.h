#pragma once

#include "ofxRulr/Nodes/Procedure/Base.h"
#include "ofxRulr/Utils/SolveSet.h"
#include "ofxMultiTrack/Frame.h"

namespace ofxRulr {
	namespace Nodes {
		namespace MultiTrack {
			namespace Procedure {
				class Calibrate : public ofxRulr::Nodes::Procedure::Base {
				public:
					enum DialogStep {
						DialogStepClosed,
						DialogStepBegin,
						DialogStepCapture,
						DialogStepComplete
					};

					struct Marker {
						ofVec2f center;
						float radius;
						ofVec3f position;
						bool valid;
					};

					Calibrate();
					string getTypeName() const override;
					void init();
					ofxCvGui::PanelPtr getPanel() override;

					void update();
					void drawWorld();
					
					void populateInspector(ofxCvGui::InspectArguments &);
					void serialize(Json::Value &);
					void deserialize(const Json::Value &);

					void dialogStepTo(DialogStep nextStep);

					void captureFrame(bool record);
					vector<Marker> findMarkersInFrame(const ofxMultiTrack::Frame & frame);
					bool mapMarkerToWorld(Marker & marker, int frameWidth, int frameHeight, const unsigned short * depthData, const float * lutData);

					void setupSolveSets();

					void applyTransforms();

				protected:
					chrono::system_clock::duration getTimeSinceCaptureStarted() const;

					shared_ptr<ofxCvGui::Panels::Widgets> panel;
					ofxCvGui::PanelPtr dialog;
					DialogStep dialogStep;

					chrono::system_clock::time_point captureStartTime;
					map<size_t, vector<Marker>> dataToPreview;
					map<size_t, map<size_t, Marker>> dataToSolve;
					map<size_t, ofxRulr::Utils::SolveSet> solveSets;

					ofTexture infrared;
					ofImage threshold;
					float error;

					struct : ofParameterGroup {
						struct : ofParameterGroup {
							ofParameter<float> duration { "Duration", 30.0f, 1.0f, 120.0f };
							PARAM_DECLARE("Capture", duration);
						} capture;

						struct : ofParameterGroup {
							ofParameter<float> threshold{ "Threshold", 100, 0, 255 };
							ofParameter<float> minimumArea{ "Minimum area", 3 * 3 };
							ofParameter<float> clipNear{ "Clip near (m)", 0.4, 0.0, 8.0 };
							ofParameter<float> clipFar{ "Clip far (m)", 8.0, 0.0, 8.0 };
							PARAM_DECLARE("Find Marker", threshold, minimumArea, clipNear, clipFar);
						} findMarker;

						struct : ofParameterGroup {
							ofParameter<bool> liveMarker{ "Live Marker", false };
							ofParameter<bool> drawPoints{ "Draw Points", true };
							ofParameter<bool> drawLines{ "Draw Lines", true };
							PARAM_DECLARE("Debug World", liveMarker, drawPoints, drawLines);
						} debugWorld;

						PARAM_DECLARE("Calibrate", capture, findMarker, debugWorld);
					} parameters;
				};
			}
		}
	}
}

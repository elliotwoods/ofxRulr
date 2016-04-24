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
					enum Step {
						StepIdle = 0,
						StepBegin,
						StepCapture,
						StepSolve,

						NumSteps
					};

					struct Marker {
						ofVec2f center;
						float radius;
						ofVec3f position;
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

					void goToStep(Step nextStep);

					void captureFrame(bool record);
					vector<Marker> findMarkersInFrame(const ofxMultiTrack::Frame & frame);

					void setupSolveSets();
					void triggerSolvers();

					void applyTransforms();

					const ofColor & getSubscriberColor(size_t key);

				protected:
					chrono::system_clock::duration getTimeSinceCaptureStarted() const;

					shared_ptr<ofxCvGui::Panels::Widgets> panel;
					ofxCvGui::PanelPtr dialog;
					Step currStep;

					chrono::system_clock::time_point captureStartTime;
					map<size_t, ofColor> subscriberColors;
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
							ofParameter<float> minimumArea{ "Minimum area", 5 * 5 };
							ofParameter<float> markerPadding{ "Marker padding [%]", 50, 0, 100 };
							PARAM_DECLARE("Find Marker", threshold, minimumArea);
						} findMarker;

						struct : ofParameterGroup {
							ofParameter<bool> drawPoints{ "Draw Points", true };
							ofParameter<bool> drawLines{ "Draw Lines", true };
							PARAM_DECLARE("Debug World", drawPoints, drawLines);
						} debugWorld;

						PARAM_DECLARE("Calibrate", capture, findMarker, debugWorld);
					} parameters;
				};
			}
		}
	}
}
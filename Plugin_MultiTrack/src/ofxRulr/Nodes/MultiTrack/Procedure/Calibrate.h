#pragma once

#include "ofxRulr/Nodes/Procedure/Base.h"

namespace ofxRulr {
	namespace Nodes {
		namespace MultiTrack {
			namespace Procedure {
				class Calibrate : public ofxRulr::Nodes::Procedure::Base {
				public:
					Calibrate();
					string getTypeName() const override;
					void init();
					ofxCvGui::PanelPtr getPanel() override;
					void update();

					void addCapture();
					void solveAll();

				protected:
					void populateInspector(ofxCvGui::InspectArguments &);
					void Calibrate::captureToggled(bool &);
					chrono::system_clock::duration getTimeSinceCaptureStarted() const;

					chrono::system_clock::time_point captureStartTime;
					map<size_t, map<string, vector<ofVec3f>>> markerData;
					float error;

					shared_ptr<ofxCvGui::Panels::Groups::Strip> panel;

					struct : ofParameterGroup {
						struct : ofParameterGroup {
							ofParameter<bool> enabled { "Enabled", false };
							ofParameter<float> duration { "Duration", 30.0f, 1.0f, 120.0f };
							PARAM_DECLARE("Capture", enabled, duration);
						} capture;

						struct : ofParameterGroup {
							ofParameter<bool> enabled{ "Enabled", true };
							ofParameter<float> threshold{ "Threshold", 100, 0, 255 };
							ofParameter<float> minimumArea{ "Minimum area", 5 * 5 };
							ofParameter<float> markerPadding{ "Marker padding [%]", 50, 0, 100 };
							PARAM_DECLARE("FindMarker", enabled, threshold, minimumArea);
						} findMarker;

						struct : ofParameterGroup {
							ofParameter<bool> trimOutliers{ "Trim Outliers", true };
							PARAM_DECLARE("SolveTransform", trimOutliers);
						} solveTransform;

						PARAM_DECLARE("Calibrate", capture, findMarker, solveTransform);
					} parameters;
				};
			}
		}
	}
}
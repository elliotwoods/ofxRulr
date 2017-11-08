#pragma once

#include "ofxRulr/Nodes/Base.h"
#include "ofxRulr/Utils/CaptureSet.h"

namespace ofxRulr {
	namespace Nodes {
		namespace Procedure {
			namespace Calibrate {
				class ProjectorFromStereoCamera : public Base {
				public:
					class Capture : public Utils::AbstractCaptureSet::BaseCapture {
					public:
						Capture();
						string getDisplayString() const override;
						void serialize(Json::Value &);
						void deserialize(const Json::Value &);

						void drawWorldStage();

						//vertex = world; texcoord = projector pixel position
						ofMesh dataPoints;
					};

					ProjectorFromStereoCamera();
					string getTypeName() const override;

					void init();
					void update();
					void drawWorldStage();

					void serialize(Json::Value &);
					void deserialize(const Json::Value &);
					void populateInspector(ofxCvGui::InspectArguments &);

					ofxCvGui::PanelPtr getPanel() override;
				protected:
					void addCapture();
					void calibrate();

					Utils::CaptureSet<Capture> captures;
					ofxCvGui::PanelPtr panel;

					struct : ofParameterGroup {
						struct : ofParameterGroup {
							ofParameter<float> threshold{ "Threshold", 5, 0, 255 };
							ofParameter<float> brightness{ "Brightness", 255, 0, 255 };
							ofParameter<float> captureDelay{ "Capture delay [ms]", 100, 0, 10000 };
							ofParameter<bool> correctStereoMatches{ "Correct stereo matches", false };
							PARAM_DECLARE("Scan", threshold, captureDelay, brightness, correctStereoMatches);
						} scan;

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
							ofParameter<float> brightnessBoost{ "Brightness boost", 0.2, 0.0, 1.0 };
							PARAM_DECLARE("Preview", brightnessBoost);
						} preview;

						PARAM_DECLARE("ProjectorFromStereoCamera", scan, calibrate, preview);
					} parameters;

					ofParameter<float> reprojectionError{ "Reprojection error [px]", 0.0f };
				};
			}
		}
	}
}
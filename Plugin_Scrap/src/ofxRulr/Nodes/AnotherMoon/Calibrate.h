#pragma once

#include "ofxRulr.h"

namespace ofxRulr {
	namespace Nodes {
		namespace AnotherMoon {
			class Calibrate : public Nodes::Base {
			public:
				MAKE_ENUM(LaserState
					, (Off, TestPattern)
					, ("Off", "TestPattern"));

				class BeamCapture : public Utils::AbstractCaptureSet::BaseCapture
				{
				public:
					BeamCapture();
					string getDisplayString() const override;
					void serialize(nlohmann::json&);
					void deserialize(const nlohmann::json&);

					glm::vec2 imagePoint;
					string urlOnImage;
					string urlOffImage;
				};

				class LaserCapture : public Utils::AbstractCaptureSet::BaseCapture
				{
				public:
					LaserCapture();
					string getDisplayString() const override;
					void serialize(nlohmann::json&);
					void deserialize(const nlohmann::json&);

					int laserAddress;
					Utils::CaptureSet<BeamCapture> beamCaptures;
				};

				class CameraCapture : public Utils::AbstractCaptureSet::BaseCapture
				{
				public:
					CameraCapture();
					string getDisplayString() const override;
					void serialize(nlohmann::json&);
					void deserialize(const nlohmann::json&);

					Utils::CaptureSet<LaserCapture> laserCaptures;
				};

				Calibrate();
				string getTypeName() const override;

				void init();

				void populateInspector(ofxCvGui::InspectArguments&);
				void serialize(nlohmann::json&);
				void deserialize(const nlohmann::json&);

				ofxCvGui::PanelPtr getPanel() override;

				void capture();
			protected:
				struct : ofParameterGroup {
					struct : ofParameterGroup {
						struct : ofParameterGroup {
							ofParameter<float> radius{ "Radius", 0.5, 0, 1.0 };
							ofParameter<int> resolution{ "Resolution", 3, 2, 4 };
							PARAM_DECLARE("Image points"
								, radius
								, resolution);
						} imagePoints;

						struct : ofParameterGroup {
							ofParameter<string> hostname{ "Hostname", "10.0.0.180" };
							ofParameter<float> captureTimeout{ "Capture timeout [s]", 10.0f };
							PARAM_DECLARE("Remote camera"
								, hostname
								, captureTimeout);
						} remoteCamera;

						ofParameter<LaserState> laserStateForOthers{ "State for others", LaserState::TestPattern };
						ofParameter<float> outputDelay{ "Output delay [s]", 0.5, 0, 5.0 };
						ofParameter<bool> continueOnFail{ "Continue on fail", false };
						ofParameter<int> signalSends{ "Signal sends", 10 };

						struct : ofParameterGroup {
							ofParameter<bool> enabled{ "Enabled", true };
							PARAM_DECLARE("Dry run", enabled);
						} dryRun;

						PARAM_DECLARE("Capture"
							, imagePoints
							, remoteCamera
							, laserStateForOthers
							, outputDelay
							, continueOnFail
							, signalSends
							, dryRun);
					} capture;
					PARAM_DECLARE("Calibrate", capture);
				} parameters;

				vector<glm::vec2> getCalibrationImagePoints() const;
				void waitForDelay() const;
				string getBaseCameraURL() const;

				string captureToURL();

				void takePhoto();
				vector<string> pollNewCameraFiles();

				ofxCvGui::PanelPtr panel;

				Utils::CaptureSet<CameraCapture> cameraCaptures;
				ofURLFileLoader urlLoader;
			};
		}
	}
}
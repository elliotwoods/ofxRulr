#pragma once

#include "ofxRulr/Nodes/Base.h"
#include "ofxRulr/Utils/CaptureSet.h"
#include "ofxOsc.h"

namespace ofxRulr {
	namespace Nodes {
		namespace Procedure {
			namespace Calibrate {
				class LaserToWorld : public Nodes::Base {
				public:
					class Capture : public Utils::AbstractCaptureSet::BaseCapture {
					public:
						Capture();
						ofParameter<ofVec3f> worldPosition{ "World", ofVec3f() };
						ofParameter<ofVec2f> projected{ "Projected", ofVec2f() };

						string getDisplayString() const override;
						void serialize(Json::Value &);
						void deserialize(const Json::Value &);
					};

					LaserToWorld();
					string getTypeName() const override;

					void init();
					void update();
					void drawWorldStage();

					ofxCvGui::PanelPtr getPanel() override;

					void serialize(Json::Value &);
					void deserialize(const Json::Value &);
					void populateInspector(ofxCvGui::InspectArguments &);
				protected:
					void addCapture();
					void calibrate();
					void testCalibration();

					ofVec3f getWorldCursorPosition() const;

					ofxCvGui::PanelPtr panel;
					Utils::CaptureSet<Capture> captures;

					struct : ofParameterGroup {
						struct : ofParameterGroup {
							ofParameter<string> remoteHost{ "Remote host", "localhost" };
							ofParameter<int> remotePort{ "Remote port", 5000 };
							PARAM_DECLARE("OSC", remoteHost, remotePort);
						} osc;
						
						struct : ofParameterGroup {
							ofParameter<bool> enabled{ "Enabled", true };
							ofParameter<float> x{ "X", 0, -1, 1 };
							ofParameter<float> y{ "Y", 0, -1, 1 };
							PARAM_DECLARE("Target", enabled, x, y);
						} target;

						struct : ofParameterGroup {
							ofParameter<float> outputDelay{ "Output delay [s]", 0.1, 0, 10 };
							ofParameter<FindBoardMode> findBoardMode{ "Find board mode", FindBoardMode::Optimized };
							ofParameter<ofVec2f> offsetSquares{ "Offset squares", ofVec2f(1, 1) };
							PARAM_DECLARE("Capture", outputDelay, findBoardMode, offsetSquares);
						} capture;

						struct : ofParameterGroup {
							ofParameter<float> initialLensOffset{ "Initial lens offset", 0, -1.0, 1.0 };
							ofParameter<float> initialThrowRatio{ "Initial throw ratio", 1, 0, 5 };
							ofParameter<bool> fixAspectRatio{ "Fix aspect ratio", true };
							PARAM_DECLARE("Calibrate", initialLensOffset, initialThrowRatio, fixAspectRatio);
						} calibrate;

						PARAM_DECLARE("LaserToWorld", osc, target, capture, calibrate);
					} parameters;

					ofParameter<float> reprojectionError{ "Reprojection error [px]", 0.0f };

					unique_ptr<ofxOscSender> oscSender;
					string cachedRemoteHost;
					int cachedRemotePort = -1;
				};
			}
		}
	}
}
#pragma once

#include "ofxRulr.h"
#include "ofxRulr/Utils/CaptureSet.h"
#include "ofxOsc.h"

#include "Laser.h"

namespace ofxRulr {
	namespace Nodes {
		namespace AnotherMoon {
			/// <summary>
			/// A set of lasers stored locally. Each laser is a BaseCapture.
			/// Calibration data is stored externally in Calibrate node
			/// </summary>
			class Lasers : public Nodes::Base {
			public:
				Lasers();
				string getTypeName() const override;
				
				void init();
				void update();
				void drawWorldStage();

				void serialize(nlohmann::json&);
				void deserialize(const nlohmann::json&);
				void populateInspector(ofxCvGui::InspectArguments&);

				ofxCvGui::PanelPtr getPanel() override;

				void pushState();
				
				void sendTestImageTo(const std::vector<shared_ptr<Laser>>);
				void sendTestImageToAll();

				void setSize(float);
				void setBrightness(float);

				void setDefaultSettings();

				vector<shared_ptr<Laser>> getSelectedLasers();
				shared_ptr<Laser> findLaser(int address);
			protected:
				friend Laser;

				void importCSV();

				Utils::CaptureSet<Laser> lasers;
				shared_ptr<ofxCvGui::Panels::Widgets> panel;

				struct : ofParameterGroup {
					ofParameter<string> baseAddress{ "Base address", "10.0.1." };
					ofParameter<int> remotePort{ "Remote port", 4000 };
					ofParameter<bool> signalEnabled{ "Signal enabled", false };
					ofParameter<Laser::State> selectedState{"Selected state", Laser::State::Standby};
					ofParameter<Laser::State> deselectedState{"Deselected state", Laser::State::Shutdown};
					ofParameter<bool> sendToDeselected{ "Send to deselected", false };
					
					struct : ofParameterGroup {
						ofParameter<bool> scheduled{ "Scheduled", true };
						ofParameter<float> updatePeriod{ "Update period [s]", 1.0f };
						PARAM_DECLARE("Push state", scheduled, updatePeriod);
					} pushState;

					struct : ofParameterGroup {
						ofParameter<bool> enabled{ "Enabled", false };
						ofParameter<float> circleRadius{ "Circle radius", 0.5f };

						struct : ofParameterGroup {
							ofParameter<bool> enabled{ "Enabled", false };
							ofParameter<float> movementRadius{ "Movement radius", 0.5f };
							ofParameter<float> period{ "Period", 10.0f };
							PARAM_DECLARE("Animation", enabled, movementRadius, period);
						} animation;
						
						PARAM_DECLARE("Test image", enabled, circleRadius, animation);
					} testImage;

					PARAM_DECLARE("Laser"
						, baseAddress
						, signalEnabled
						, selectedState
						, deselectedState
						, sendToDeselected
						, pushState
						, testImage);
				} parameters;

				chrono::system_clock::time_point lastPushStateTime = chrono::system_clock::now();
			};
		}
	}
}
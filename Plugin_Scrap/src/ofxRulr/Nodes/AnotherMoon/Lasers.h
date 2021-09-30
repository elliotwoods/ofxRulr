#pragma once

#include "ofxRulr.h"
#include "ofxRulr/Utils/CaptureSet.h"
#include "ofxOsc.h"

namespace ofxRulr {
	namespace Nodes {
		namespace AnotherMoon {
			class Lasers : public Nodes::Base {
			public:
				MAKE_ENUM(State
					, (Shutdown, Standby, Run)
					, ("Shutdown", "Standby", "Run"));

				MAKE_ENUM(Source
					, (Circle, USB, Memory)
					, ("Circle", "USB", "Memory"));

				class Laser : public Utils::AbstractCaptureSet::BaseCapture, public ofxCvGui::IInspectable {
				public:
					Laser();
					~Laser();
					void setParent(Lasers*);

					string getDisplayString() const override;
					void serialize(nlohmann::json&);
					void deserialize(const nlohmann::json&);
					void populateInspector(ofxCvGui::InspectArguments&);
					void drawWorldStage();

					glm::mat4 getTransform() const;

					void shutown();
					void standby();
					void run();

					void setBrightness(float);
					void setSize(float);
					void setSource(const Source&);

					void drawCircle(glm::vec2 center, float radius);

					string getHostname() const;
					void sendMessage(const ofxOscMessage&);

					struct : ofParameterGroup {
						struct : ofParameterGroup {
							ofParameter<int> address{ "Address", 0 };
							ofParameter<glm::vec3> position{ "Position", {0, 0, 0} };
							ofParameter<glm::vec3> rotation{ "Rotation", {0, 0,0} };
							ofParameter<glm::vec2> fov{ "FOV", {30, 30} };
							ofParameter<glm::vec2> centerOffset{ "Center offset", {0, 0} };
							PARAM_DECLARE("Settings", address, position,rotation, fov, centerOffset);
						} settings;

						PARAM_DECLARE("Laser", settings);
					} parameters;
				protected:
					Lasers * parent;
					unique_ptr<ofxOscSender> oscSender;
				};

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
			protected:
				void importCSV();

				Utils::CaptureSet<Laser> lasers;
				shared_ptr<ofxCvGui::Panels::Widgets> panel;

				struct : ofParameterGroup {
					ofParameter<string> baseAddress{ "Base address", "10.0.1." };
					ofParameter<int> remotePort{ "Remote port", 4000 };
					ofParameter<bool> signalEnabled{ "Signal enabled", false };
					ofParameter<State> selectedState{"Selected state", State::Standby};
					ofParameter<State> deselectedState{"Deselected state", State::Shutdown};
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
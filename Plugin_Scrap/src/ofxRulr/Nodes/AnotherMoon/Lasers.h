#pragma once

#include "ofxRulr.h"
#include "ofxRulr/Utils/CaptureSet.h"
#include "ofxRulr/Data/AnotherMoon/MessageRouter.h"

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
				void drawWorldAdvanced(DrawWorldAdvancedArgs&);

				void serialize(nlohmann::json&);
				void deserialize(const nlohmann::json&);
				void populateInspector(ofxCvGui::InspectArguments&);

				ofxCvGui::PanelPtr getPanel() override;

				void setStateBySelection();
				void pushAllSelected();

				void sendTestImageTo(const std::vector<shared_ptr<Laser>>);
				void sendTestImageToSelected();

				vector<shared_ptr<Laser>> getLasersSelected();
				vector<shared_ptr<Laser>> getLasersAll();

				shared_ptr<Laser> findLaser(int serialNumber, bool onlySelected);

				bool sendMessage(shared_ptr<Data::AnotherMoon::OutgoingMessage>);
				bool isCommunicationEnabled() const;
			protected:
				friend Laser;

				void importJson();

				Utils::CaptureSet<Laser> lasers;
				shared_ptr<ofxCvGui::Panels::Widgets> panel;

				Data::AnotherMoon::MessageRouter messageRouter;

				struct : ofParameterGroup {
					struct : ofParameterGroup {
						ofParameter<bool> enabled{ "Enabled", true };
						ofParameter<bool> logAcks{ "Log acks", false };
						ofParameter<int> retryDuration{ "Retry duration [ms]", 10000 };
						ofParameter<int> retryPeriod{ "Retry period [ms]", 500 };
						PARAM_DECLARE("Communications", enabled, logAcks, retryDuration, retryPeriod);
					} communications;

					struct : ofParameterGroup {
						ofParameter<bool> enabled{ "Enabled", true };
						ofParameter<Laser::State> selectedState{ "Selected state", Laser::State::Standby };
						ofParameter<Laser::State> deselectedState{ "Deselected state", Laser::State::Shutdown };
						PARAM_DECLARE("Set state by selected", enabled, selectedState, deselectedState);
					} setStateBySelected;
					
					struct : ofParameterGroup {
						ofParameter<bool> enabled{ "Enabled", true };
						ofParameter<float> updatePeriod{ "Update period [s]", 10.0f, 0, 60.0f };
						PARAM_DECLARE("Push full state", enabled, updatePeriod);
					} pushFullState;

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

					struct : ofParameterGroup {
						ofParameter<WhenActive> rigidBody{ "Rigid Body", WhenActive::Selected };
						ofParameter<WhenActive> trussLine{ "Truss line", WhenActive::Always };
						ofParameter<WhenActive> centerLine{ "Center line", WhenActive::Selected };
						ofParameter<WhenActive> centerOffsetLine{ "Center offset line", WhenActive::Never };
						ofParameter<WhenActive> modelPreview{ "Model preview", WhenActive::Always };
						ofParameter<WhenActive> frustum{ "Frustum", WhenActive::Selected };
						ofParameter<WhenActive> picturePreview{ "Picture preview", WhenActive::Selected };
						PARAM_DECLARE("Draw", rigidBody, trussLine, centerLine, centerOffsetLine, modelPreview, frustum, picturePreview);
					} draw;

					PARAM_DECLARE("Lasers"
						, communications
						, setStateBySelected
						, pushFullState
						, testImage
						, draw);
				} parameters;

				chrono::system_clock::time_point lastPushAllTime = chrono::system_clock::now();
			};
		}
	}
}
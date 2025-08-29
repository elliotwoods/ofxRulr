#pragma once

#include "ofxRulr.h"
#include "Calibrate.h"

namespace ofxRulr {
	namespace Nodes {
		namespace Reworld {
			class CalibrateController : public Base
			{
			public:
				CalibrateController();
				string getTypeName() const override;

				void init();
				void update();
				void populateInspector(ofxCvGui::InspectArguments);
				void remoteControl(RemoteControllerArgs);

				ofxCvGui::PanelPtr getPanel() override;

				void up();
				void down();
				void left();
				void right();

				void moveAxes(const glm::vec2 &);

				void selectClosestModuleToMouseCursor();
			protected:
				void rebuildPanel();

				struct ButtonState {
					bool up = false;
					bool down = false;
					bool left = false;
					bool right = false;
				};
				ButtonState priorButtonState;

				struct : ofParameterGroup {
					ofParameter<float> movementSpeed{ "Movement speed", -0.3, -1, 1 };
					ofParameter<float> movementPower{ "Movement power", 4, 1, 10 };
					PARAM_DECLARE("CalibrateController", movementSpeed, movementPower);
				} parameters;

				weak_ptr<Calibrate::CalibrateControllerSession> calibrateControllerSession;
				shared_ptr<ofxCvGui::Panels::Widgets> panel;
			};
		}
	}
}
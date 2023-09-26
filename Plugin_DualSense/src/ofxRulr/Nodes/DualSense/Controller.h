#pragma once

#include "ofxRulr/Nodes/Base.h"
#include "ofxDualSense.h"

namespace ofxRulr {
	namespace Nodes {
		namespace DualSense {
			class Controller : public Base
			{
			public:
				Controller();
				string getTypeName() const override;

				void init();
				void update();

				void setDeviceIndex(size_t);

				ofxCvGui::PanelPtr getPanel() override;

			protected:
				void refreshPanel();

				struct : ofParameterGroup {
					ofParameter<int> index{ "Index", -1 };
					ofParameter<float> deadZone{ "Dead zone", 0.1f };
					ofParameter<float> speed{ "Speed", 1.0f, 0.01f, 100.0f };

					PARAM_DECLARE("Controller", index, deadZone, speed);
				} parameters;

				shared_ptr<ofxCvGui::Panels::Groups::Strip> panel;

				shared_ptr<ofxDualSense::Controller> controller;
				ofxDualSense::InputState inputState;
				ofxDualSense::InputState priorInputState;
			};
		}
	}
}
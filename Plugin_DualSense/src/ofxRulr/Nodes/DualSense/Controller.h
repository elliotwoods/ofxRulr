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
				void populateInspector(ofxCvGui::InspectArguments&);

				void setDeviceIndex(size_t);
				void closeDevice();

				ofxCvGui::PanelPtr getPanel() override;
			protected:
				void refreshPanel();

				struct : ofParameterGroup {
					ofParameter<int> index{ "Index", -1 };
					ofParameter<float> deadZone{ "Dead zone", 0.1f };
					ofParameter<float> coarseSpeed{ "Coarse speed", 0.1f, 0.01f, 100.0f };
					ofParameter<float> fineSpeed{ "Fine speed", 0.01f, 0.01f, 100.0f };
					ofParameter<float> activeTime{ "Active time", 5.0f, 0.0f, 60.0f };

					PARAM_DECLARE("Controller", index, deadZone, coarseSpeed, fineSpeed, activeTime);
				} parameters;

				shared_ptr<ofxCvGui::Panels::Groups::Strip> panel;

				shared_ptr<ofxDualSense::Controller> controller;
				ofxDualSense::InputState inputState;
				ofxDualSense::InputState priorInputState;

				chrono::system_clock::time_point lastActivated = chrono::system_clock::now();
				chrono::system_clock::time_point lastActivatedBegin = chrono::system_clock::now();
				bool isActive = false;
			};
		}
	}
}
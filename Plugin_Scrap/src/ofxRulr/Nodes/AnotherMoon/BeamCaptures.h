#pragma once

#include "ofxRulr.h"
#include "Calibrate.h"

namespace ofxRulr {
	namespace Nodes {
		namespace AnotherMoon {
			class BeamCaptures : public Nodes::Base {
			public:
				BeamCaptures();
				string getTypeName() const override;

				void init();
				void update();

				ofxCvGui::PanelPtr getPanel() override;
				Calibrate::LaserCapture* laserCapture = nullptr;
			protected:
				void rebuildView();
				shared_ptr<ofxCvGui::Panels::Widgets> panel;
			};
		}
	}
}
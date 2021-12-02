#pragma once

#include "ofxRulr.h"
#include "Calibrate.h"

namespace ofxRulr {
	namespace Nodes {
		namespace AnotherMoon {
			class LaserCaptures : public Nodes::Base {
			public:
				LaserCaptures();
				string getTypeName() const override;

				void init();
				void update();

				ofxCvGui::PanelPtr getPanel() override;
				Calibrate::CameraCapture* cameraCapture = nullptr;
			protected:
				void rebuildView();
				shared_ptr<ofxCvGui::Panels::Widgets> panel;
			};
		}
	}
}
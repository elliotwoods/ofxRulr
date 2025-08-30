#pragma once

#include "ofxRulr/Nodes/IHasVertices.h"
#include "ofxRulr/Data/Reworld/Capture.h"
#include "ofxRulr/Utils/EditSelection.h"

#include "Calibrate.h"

namespace ofxRulr {
	namespace Nodes {
		namespace Reworld {
			class CaptureView : public Base
			{
			public:
				CaptureView();
				~CaptureView();
				string getTypeName() const override;

				void init();
				void update();
				void populateInspector(ofxCvGui::InspectArguments&);

				ofxCvGui::PanelPtr getPanel() override;
				Data::Reworld::Capture* selection = nullptr;

				static bool isSelected(Data::Reworld::Capture*);
			protected:
				void rebuildView();
				static set<CaptureView*> instances;
				shared_ptr<ofxCvGui::Panels::Groups::Strip> panel;
			};
		}
	}
}
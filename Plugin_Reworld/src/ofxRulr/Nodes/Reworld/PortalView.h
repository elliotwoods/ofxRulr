#pragma once

#include "ofxRulr/Nodes/IHasVertices.h"
#include "ofxRulr/Data/Reworld/Column.h"
#include "ofxRulr/Utils/EditSelection.h"

#include "Installation.h"

namespace ofxRulr {
	namespace Nodes {
		namespace Reworld {
			class PortalView : public Base
			{
			public:
				PortalView();
				string getTypeName() const override;

				void init();
				void update();

				ofxCvGui::PanelPtr getPanel() override;
				Data::Reworld::Portal* selection = nullptr;
			protected:
				void rebuildView();
				shared_ptr<ofxCvGui::Panels::Widgets> panel;
			};
		}
	}
}
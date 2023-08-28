#pragma once

#include "ofxRulr/Nodes/IHasVertices.h"
#include "ofxRulr/Data/Reworld/Panel.h"
#include "ofxRulr/Utils/EditSelection.h"

#include "Installation.h"

namespace ofxRulr {
	namespace Nodes {
		namespace Reworld {
			class PanelView : public Base
			{
			public:
				PanelView();
				string getTypeName() const override;

				void init();
				void update();

				ofxCvGui::PanelPtr getPanel() override;
				Data::Reworld::Panel* selection = nullptr;
			protected:
				void rebuildView();
				shared_ptr<ofxCvGui::Panels::Widgets> panel;
			};
		}
	}
}
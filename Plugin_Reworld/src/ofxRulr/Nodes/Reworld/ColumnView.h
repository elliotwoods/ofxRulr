#pragma once

#include "ofxRulr/Nodes/IHasVertices.h"
#include "ofxRulr/Data/Reworld/Column.h"
#include "ofxRulr/Utils/EditSelection.h"

#include "Installation.h"

namespace ofxRulr {
	namespace Nodes {
		namespace Reworld {
			class ColumnView : public Base
			{
			public:
				ColumnView();
				~ColumnView();
				string getTypeName() const override;

				void init();
				void update();

				ofxCvGui::PanelPtr getPanel() override;
				Data::Reworld::Column * selection = nullptr;

				static bool isSelected(Data::Reworld::Column *);
			protected:
				void rebuildView();
				static set<ColumnView*> instances;
				shared_ptr<ofxCvGui::Panels::Widgets> panel;
			};
		}
	}
}
#pragma once

#include "ofxRulr/Nodes/IHasVertices.h"
#include "ofxRulr/Data/Reworld/Column.h"
#include "ofxRulr/Utils/EditSelection.h"

#include "Installation.h"

namespace ofxRulr {
	namespace Nodes {
		namespace Reworld {
			class ModuleView : public Base
			{
			public:
				ModuleView();
				~ModuleView();

				string getTypeName() const override;

				void init();
				void update();

				ofxCvGui::PanelPtr getPanel() override;
				Data::Reworld::Module* selection = nullptr;

				static bool isSelected(Data::Reworld::Module*);
			protected:
				void rebuildView();
				shared_ptr<ofxCvGui::Panels::Widgets> panel;
				static set<ModuleView*> instances;
			};
		}
	}
}
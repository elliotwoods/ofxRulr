#include "pch_Plugin_Reworld.h"
#include "ModuleView.h"
#include "ColumnView.h"

namespace ofxRulr {
	namespace Nodes {
		namespace Reworld {
			//----------
			set<ModuleView*> ModuleView::instances;

			//----------
			ModuleView::ModuleView()
			{
				ModuleView::instances.insert(this);

				RULR_NODE_INIT_LISTENER;
			}

			//----------
			ModuleView::~ModuleView()
			{
				ModuleView::instances.erase(this);
			}

			//----------
			string
				ModuleView::getTypeName() const
			{
				return "Reworld::ModuleView";
			}

			//----------
			void
				ModuleView::init()
			{
				RULR_NODE_UPDATE_LISTENER;

				auto input = this->addInput<ColumnView>();
				this->panel = ofxCvGui::Panels::makeWidgets();

				this->rebuildView();
			}

			//----------
			void
				ModuleView::update()
			{
				auto priorSelection = this->selection;

				// Update current selection
				{
					this->selection = nullptr;
					auto inputNode = this->getInput<ColumnView>();
					if (inputNode) {
						if (inputNode->selection) {
							this->selection = inputNode->selection->ourSelection.selection;
						}
					}
				}

				if (this->selection != priorSelection) {
					this->rebuildView();
				}
			}

			//----------
			ofxCvGui::PanelPtr
				ModuleView::getPanel()
			{
				return this->panel;
			}

			//----------
			bool
				ModuleView::isSelected(Data::Reworld::Module* module)
			{
				for (auto instance : ModuleView::instances) {
					if (instance->selection == module) {
						return true;
					}
				}
				return false;
			}

			//----------
			void
				ModuleView::rebuildView()
			{
				this->panel->clear();
				if (!this->selection) {
					this->panel->addTitle("Select module first");
				}
				else {
					this->panel->addParameterGroup(this->selection->parameters);
				}
			}
		}
	}
}
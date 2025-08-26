#include "pch_Plugin_Reworld.h"
#include "ColumnView.h"

namespace ofxRulr {
	namespace Nodes {
		namespace Reworld {
			//----------
			set<ColumnView*> ColumnView::instances;

			//----------
			ColumnView::ColumnView()
			{
				ColumnView::instances.insert(this);

				RULR_NODE_INIT_LISTENER;
			}

			//----------
			ColumnView::~ColumnView()
			{
				ColumnView::instances.erase(this);
			}

			//----------
			string
				ColumnView::getTypeName() const
			{
				return "Reworld::ColumnView";
			}

			//----------
			void
				ColumnView::init()
			{
				RULR_NODE_UPDATE_LISTENER;

				auto installationInput = this->addInput<Installation>();
				this->panel = ofxCvGui::Panels::makeWidgets();

				this->rebuildView();
			}

			//----------
			void
				ColumnView::update()
			{
				auto priorSelection = this->selection;

				// Update current selection
				{
					auto inputNode = this->getInput<Installation>();
					if (!inputNode) {
						this->selection = nullptr;
					}
					else {
						this->selection = inputNode->ourSelection.selection;
					}
				}

				if (this->selection != priorSelection) {
					this->rebuildView();
				}
			}

			//----------
			ofxCvGui::PanelPtr
				ColumnView::getPanel()
			{
				return this->panel;
			}

			//----------
			bool
				ColumnView::isSelected(Data::Reworld::Column* column)
			{
				for (auto instance : ColumnView::instances) {
					if (instance->selection == column) {
						return true;
					}
				}
				return false;
			}

			//----------
			void
				ColumnView::rebuildView()
			{
				this->panel->clear();
				if (!this->selection) {
					this->panel->addTitle("Select column first");
				}
				else {
					panel->addTitle("Modules:", ofxCvGui::Widgets::Title::Level::H3);
					this->selection->modules.populateWidgets(this->panel);
				}
			}
		}
	}
}
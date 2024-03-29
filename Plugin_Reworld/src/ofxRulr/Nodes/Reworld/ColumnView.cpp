#include "pch_Plugin_Reworld.h"
#include "ColumnView.h"

namespace ofxRulr {
	namespace Nodes {
		namespace Reworld {
			//----------
			ColumnView::ColumnView()
			{
				RULR_NODE_INIT_LISTENER;
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
			void
				ColumnView::rebuildView()
			{
				this->panel->clear();
				if (!this->selection) {
					this->panel->addTitle("Select column first");
				}
				else {
					panel->addTitle("Panels:", ofxCvGui::Widgets::Title::Level::H3);
					this->selection->panels.populateWidgets(this->panel);
				}
			}
		}
	}
}
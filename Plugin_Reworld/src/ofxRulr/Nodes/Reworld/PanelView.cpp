#include "pch_Plugin_Reworld.h"
#include "PanelView.h"
#include "ColumnView.h"

namespace ofxRulr {
	namespace Nodes {
		namespace Reworld {
			//----------
			PanelView::PanelView()
			{
				RULR_NODE_INIT_LISTENER;
			}

			//----------
			string
				PanelView::getTypeName() const
			{
				return "Reworld::PanelView";
			}

			//----------
			void
				PanelView::init()
			{
				RULR_NODE_UPDATE_LISTENER;

				auto input = this->addInput<ColumnView>();
				this->panel = ofxCvGui::Panels::makeWidgets();

				this->rebuildView();
			}

			//----------
			void
				PanelView::update()
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
				PanelView::getPanel()
			{
				return this->panel;
			}

			//----------
			void
				PanelView::rebuildView()
			{
				this->panel->clear();
				if (!this->selection) {
					this->panel->addTitle("Select panel first");
				}
				else {
					this->panel->addTitle("Portals:", ofxCvGui::Widgets::Title::Level::H3);
					this->selection->portals.populateWidgets(this->panel);
				}
			}
		}
	}
}
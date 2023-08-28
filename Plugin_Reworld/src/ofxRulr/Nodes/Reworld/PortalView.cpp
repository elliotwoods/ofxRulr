#include "pch_Plugin_Reworld.h"
#include "PortalView.h"
#include "PanelView.h"

namespace ofxRulr {
	namespace Nodes {
		namespace Reworld {
			//----------
			PortalView::PortalView()
			{
				RULR_NODE_INIT_LISTENER;
			}

			//----------
			string
				PortalView::getTypeName() const
			{
				return "Reworld::PortalView";
			}

			//----------
			void
				PortalView::init()
			{
				RULR_NODE_UPDATE_LISTENER;

				auto input = this->addInput<PanelView>();
				this->panel = ofxCvGui::Panels::makeWidgets();

				this->rebuildView();
			}

			//----------
			void
				PortalView::update()
			{
				auto priorSelection = this->selection;

				// Update current selection
				{
					this->selection = nullptr;
					auto inputNode = this->getInput<PanelView>();
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
				PortalView::getPanel()
			{
				return this->panel;
			}

			//----------
			void
				PortalView::rebuildView()
			{
				this->panel->clear();
				if (!this->selection) {
					this->panel->addTitle("Select portal first");
				}
				else {
					
				}
			}
		}
	}
}
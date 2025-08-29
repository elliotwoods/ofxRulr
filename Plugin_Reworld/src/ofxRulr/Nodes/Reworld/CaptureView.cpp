#include "pch_Plugin_Reworld.h"
#include "CaptureView.h"

namespace ofxRulr {
	namespace Nodes {
		namespace Reworld {
			//----------
			set<CaptureView*> CaptureView::instances;

			//----------
			CaptureView::CaptureView()
			{
				CaptureView::instances.insert(this);

				RULR_NODE_INIT_LISTENER;
			}

			//----------
			CaptureView::~CaptureView()
			{
				CaptureView::instances.erase(this);
			}

			//----------
			string
				CaptureView::getTypeName() const
			{
				return "Reworld::CaptureView";
			}

			//----------
			void
				CaptureView::init()
			{
				RULR_NODE_UPDATE_LISTENER;

				this->addInput<Calibrate>();
				this->panel = ofxCvGui::Panels::Groups::makeStrip();

				this->rebuildView();
			}

			//----------
			void
				CaptureView::update()
			{
				auto priorSelection = this->selection;

				// Update current selection
				{
					auto inputNode = this->getInput<Calibrate>();
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

				if (this->selection) {
					this->selection->updatePanel();
				}
			}

			//----------
			ofxCvGui::PanelPtr
				CaptureView::getPanel()
			{
				return this->panel;
			}

			//----------
			bool
				CaptureView::isSelected(Data::Reworld::Capture* capture)
			{
				for (auto instance : CaptureView::instances) {
					if (instance->selection == capture) {
						return true;
					}
				}
				return false;
			}

			//----------
			void
				CaptureView::rebuildView()
			{
				this->panel->clear();
				this->panel->setDirection(ofxCvGui::Panels::Groups::Strip::Horizontal);

				if (!this->selection) {
					auto widgets = ofxCvGui::Panels::makeWidgets();
					widgets->addTitle("Select capture first");
					this->panel->add(widgets);
				}
				else {
					this->selection->populatePanel(this->panel);
				}
			}
		}
	}
}
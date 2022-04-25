#include "pch_Plugin_Scrap.h"
#include "LaserCaptures.h"

namespace ofxRulr {
	namespace Nodes {
		namespace AnotherMoon {
			//----------
			LaserCaptures::LaserCaptures()
			{
				RULR_NODE_INIT_LISTENER;
			}

			//----------
			string
				LaserCaptures::getTypeName() const
			{
				return "AnotherMoon::LaserCaptures";
			}

			//----------
			void
				LaserCaptures::init()
			{
				RULR_NODE_UPDATE_LISTENER;

				auto calibrateInput = this->addInput<Calibrate>();
				this->panel = ofxCvGui::Panels::makeWidgets();
				calibrateInput->onNewConnection += [this](shared_ptr<Calibrate> calibrate) {
					calibrate->cameraEditSelection.onSelectionChanged.addListener([this, calibrate]() {
						this->cameraCapture = calibrate->cameraEditSelection.selection;
						this->rebuildView();
					}, this);
				};
				calibrateInput->onDeleteConnection += [this](shared_ptr<Calibrate> calibrate) {
					calibrate->cameraEditSelection.onSelectionChanged.removeListeners(this);
					this->cameraCapture = nullptr;
					this->rebuildView();
				};

				this->rebuildView();
			}

			//----------
			void
				LaserCaptures::update()
			{

			}

			//----------
			ofxCvGui::PanelPtr
				LaserCaptures::getPanel()
			{
				return this->panel;
			}

			//----------
			void
				LaserCaptures::rebuildView()
			{
				this->panel->clear();
				if (!this->cameraCapture) {
					this->panel->addTitle("Select camera capture");
				}
				else {
					panel->addTitle("Lasers:", ofxCvGui::Widgets::Title::Level::H3);
					this->cameraCapture->laserCaptures.populateWidgets(this->panel);
				}
			}
		}
	}
}
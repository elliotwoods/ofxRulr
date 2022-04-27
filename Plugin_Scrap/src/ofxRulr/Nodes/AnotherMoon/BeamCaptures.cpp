#include "pch_Plugin_Scrap.h"
#include "BeamCaptures.h"
#include "LaserCaptures.h"

namespace ofxRulr {
	namespace Nodes {
		namespace AnotherMoon {
			//----------
			BeamCaptures::BeamCaptures()
			{
				RULR_NODE_INIT_LISTENER;
			}

			//----------
			string
				BeamCaptures::getTypeName() const
			{
				return "AnotherMoon::BeamCaptures";
			}

			//----------
			void
				BeamCaptures::init()
			{
				RULR_NODE_UPDATE_LISTENER;

				this->addInput<LaserCaptures>();
				this->panel = ofxCvGui::Panels::makeWidgets();

				this->rebuildView();
			}

			//----------
			void
				BeamCaptures::update()
			{
				auto priorLaserCapture = this->laserCapture;
				auto laserCapturesNode = this->getInput<LaserCaptures>();
				if (!laserCapturesNode) {
					this->laserCapture = nullptr;
				}
				else {
					auto cameraCapture = laserCapturesNode->cameraCapture;
					if (!cameraCapture) {
						this->laserCapture = nullptr;
					}
					else {
						this->laserCapture = cameraCapture->ourSelection.selection;
					}
				}

				if (this->laserCapture != priorLaserCapture) {
					this->rebuildView();
				}
			}

			//----------
			ofxCvGui::PanelPtr
				BeamCaptures::getPanel()
			{
				return this->panel;
			}

			//----------
			void
				BeamCaptures::rebuildView()
			{
				this->panel->clear();
				if (!this->laserCapture) {
					this->panel->addTitle("Select laser capture");
				}
				else {
					panel->addTitle("Laser #" + ofToString(this->laserCapture->laserAddress));
					panel->addIndicatorBool("Solve success", [this]() {
						return this->laserCapture->linesWithCommonPointSolveResult.success;
						});
					panel->addLiveValue<float>("Residual", [this]() {
						return this->laserCapture->linesWithCommonPointSolveResult.residual;
						});
					panel->addTitle("Beam positions:", ofxCvGui::Widgets::Title::Level::H3);
					panel->addParameterGroup(this->laserCapture->parameters);

					this->laserCapture->beamCaptures.populateWidgets(this->panel);
				}
			}
		}
	}
}
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

				// Because we recycle this class from elsewhere
				this->parameters.draw.setName("Draw");
				this->parameters.draw.rays.set(WhenActive::Always);
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
			void
				BeamCaptures::drawWorldStage()
			{
				if (this->laserCapture) {
					auto laserCapturesNode = this->getInput<LaserCaptures>();
					if (laserCapturesNode) {
						auto calibrateNode = laserCapturesNode->getInput<Calibrate>();
						if (calibrateNode) {
							// Setup the parameters
							Calibrate::DrawParameters drawParameters;
							{
								drawParameters.beamCaptures = this->parameters.draw;
							}
							
							// Setup the draw arguments
							Calibrate::DrawArguments args{
								this
								, calibrateNode->getInput<Item::Camera>()
								, calibrateNode->getInput<Lasers>()
								, drawParameters
							};

							this->laserCapture->drawWorldStage(args);
						}
					}
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
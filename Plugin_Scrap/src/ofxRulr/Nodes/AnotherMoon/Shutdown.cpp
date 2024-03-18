#include "pch_Plugin_Scrap.h"
#include "Shutdown.h"
#include "Lasers.h"

namespace ofxRulr {
	namespace Nodes {
		namespace AnotherMoon {
			//----------
			Shutdown::Shutdown()
			{
				RULR_NODE_INIT_LISTENER;
			}

			//----------
			string
				Shutdown::getTypeName() const
			{
				return "AnotherMoon::Shutdown";
			}

			//----------
			void
				Shutdown::init()
			{
				this->addInput<Lasers>();

				// Create panel with button
				{
					auto panel = ofxCvGui::Panels::makeBlank();
					auto button = make_shared<ofxCvGui::Widgets::Button>("Shutdown"
						, [this]() {
							this->shutdown();
						}
						, OF_KEY_ESC);

					panel->addChild(button);
					panel->onBoundsChange += [button](ofxCvGui::BoundsChangeArguments& args) {
						button->setBounds(args.localBounds);
					};
					panel->onUpdate += [button, this](ofxCvGui::UpdateArguments&) {
						auto hasLasersNode = (bool)this->getInput<Lasers>();
						button->setEnabled(hasLasersNode);
					};
					this->panel = panel;
				}
			}

			//----------
			ofxCvGui::PanelPtr
				Shutdown::getPanel()
			{
				return this->panel;
			}

			//----------
			void
				Shutdown::shutdown()
			{
				auto lasersNode = this->getInput<Lasers>();
				if (lasersNode) {
					lasersNode->shutdownAll();
				}
			}
		}
	}
}
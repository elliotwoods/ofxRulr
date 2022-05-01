#include "pch_Plugin_Scrap.h"
#include "Target.h"
#include "Lasers.h"

namespace ofxRulr {
	namespace Nodes {
		namespace AnotherMoon {
			//-----------
			Target::Target()
			{
				RULR_NODE_INIT_LISTENER;
			}

			//-----------
			string
			Target::getTypeName() const
			{
				return "AnotherMoon::Target";
			}

			//-----------
			void
			Target::init()
			{
				RULR_NODE_DRAW_WORLD_LISTENER;
				RULR_NODE_INSPECTOR_LISTENER;

				this->addInput<Item::RigidBody>();
				this->addInput<Lasers>();
			}

			//-----------
			void
			Target::drawWorldStage()
			{

			}

			//-----------
			void
				Target::populateInspector(ofxCvGui::InspectArguments& args)
			{
				auto inspector = args.inspector;
				inspector->addButton("Position target relative to lasers", [this]() {
					try {
						this->positionTargetRelativeToLasers();
					}
					RULR_CATCH_ALL_TO_ALERT;
					});
				inspector->addButton("Point lasers towards target", [this]() {
					try {
						this->pointLasersTowardsTarget();
					}
					RULR_CATCH_ALL_TO_ALERT;
					});
			}

			//-----------
			void
				Target::positionTargetRelativeToLasers()
			{
				this->throwIfMissingAnyConnection();

				// Get relative position value
				glm::vec3 relativePosition(0, -50, 0);
				{
					auto text = ofSystemTextBoxDialog("Relative position (" + ofToString(relativePosition, 1) + ")");
					if (!text.empty()) {
						stringstream ss(text);
						ss >> relativePosition;
					}
				}

				// Take mean positon
				glm::vec3 meanPosition(0, 0, 0);
				{
					auto lasers = this->getInput<Lasers>()->getSelectedLasers();
					for (auto laser : lasers) {
						meanPosition += laser->getRigidBody()->getPosition();
					}
					meanPosition /= (float)lasers.size();
				}

				auto targetPosition = meanPosition + relativePosition;
				this->getInput<Item::RigidBody>()->setPosition(targetPosition);
			}

			//-----------
			void
				Target::pointLasersTowardsTarget()
			{
				this->throwIfMissingAnyConnection();

				auto targetPosition = this->getInput<Item::RigidBody>()->getPosition();

				auto lasers = this->getInput<Lasers>()->getSelectedLasers();
				for (auto laser : lasers) {
					laser->getRigidBody()->lookAt(targetPosition);
				}
			}
		}
	}
}
#include "pch_Plugin_Reworld.h"
#include "LookAtPointToPoint.h"

#include "Installation.h"

namespace ofxRulr {
	namespace Nodes {
		namespace Reworld {
			//----------
			LookAtPointToPoint::LookAtPointToPoint()
			{
				RULR_NODE_INIT_LISTENER;
			}

			//----------
			string
				LookAtPointToPoint::getTypeName() const
			{
				return "Reworld::LookAtPointToPoint";
			}

			//----------
			void
				LookAtPointToPoint::init()
			{
				RULR_NODE_INSPECTOR_LISTENER;

				this->addInput<Installation>();
				this->addInput<Item::RigidBody>("Point 1");
				this->addInput<Item::RigidBody>("Point 2");

				this->manageParameters(this->parameters);
			}

			//----------
			void
				LookAtPointToPoint::update()
			{
				if (ofxRulr::isActive(this->isBeingInspected(), this->parameters.performAutomatically.get())) {
					try {
						this->perform();
					}
					RULR_CATCH_ALL_TO_ERROR;
				}
			}

			//----------
			void
				LookAtPointToPoint::populateInspector(ofxCvGui::InspectArguments args)
			{
				auto inspector = args.inspector;
				inspector->addButton("Perform", [this]() {
					try {
						this->perform();
					}
					RULR_CATCH_ALL_TO_ALERT;
					}, ' ')->setHeight(100.0f);
			}

			//----------
			void
				LookAtPointToPoint::perform()
			{
				this->throwIfMissingAnyConnection();

				auto installationNode = this->getInput<Installation>();
				auto point1Node = this->getInput<Item::RigidBody>("Point 1");
				auto point2Node = this->getInput<Item::RigidBody>("Point 2");

				auto point1 = point1Node->getPosition();
				auto point2 = point2Node->getPosition();
			}
		}
	}
}
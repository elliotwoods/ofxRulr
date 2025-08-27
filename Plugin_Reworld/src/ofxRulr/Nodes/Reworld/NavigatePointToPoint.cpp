#include "pch_Plugin_Reworld.h"
#include "NavigatePointToPoint.h"
#include "Installation.h"
#include "ofxRulr/Solvers/Reworld/Navigate/PointToPoint.h"

namespace ofxRulr {
	namespace Nodes {
		namespace Reworld {
			//----------
			NavigatePointToPoint::NavigatePointToPoint()
			{
				RULR_NODE_INIT_LISTENER;
			}

			//----------
			string
				NavigatePointToPoint::getTypeName() const
			{
				return "Reworld::NavigatePointToPoint";
			}

			//----------
			void
				NavigatePointToPoint::init()
			{
				RULR_NODE_INSPECTOR_LISTENER;
				RULR_NODE_UPDATE_LISTENER;

				this->addInput<Installation>();
				
				{
					for (int i = 0; i < 2; i++) {
						auto input = this->addInput<Item::RigidBody>("Point " + ofToString(i));
						input->onNewConnection += [this](shared_ptr<Item::RigidBody> rigidBody) {
							rigidBody->onTransformChange.addListener([this]() {
								this->needsPerform = true;
								}, this);
								};
						input->onDeleteConnection += [this](shared_ptr<Item::RigidBody> rigidBody) {
							rigidBody->onTransformChange.removeListeners(this);
							};
					}
				}

				this->manageParameters(this->parameters);
			}

			//----------
			void
				NavigatePointToPoint::update()
			{
				if (ofxRulr::isActive(this->isBeingInspected(), this->parameters.performAutomatically.get())) {
					try {
						this->perform();
					}
					RULR_CATCH_ALL_TO_ERROR;
				}

				if (this->needsPerform) {
					try {
						this->perform();
					}
					RULR_CATCH_ALL_TO_ERROR;
				}
			}

			//----------
			void
				NavigatePointToPoint::populateInspector(ofxCvGui::InspectArguments args)
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
				NavigatePointToPoint::perform()
			{
				this->throwIfMissingAnyConnection();

				auto installationNode = this->getInput<Installation>();
				auto point1Node = this->getInput<Item::RigidBody>("Point 1");
				auto point2Node = this->getInput<Item::RigidBody>("Point 2");

				auto point1 = point1Node->getPosition();
				auto point2 = point2Node->getPosition();

				auto modules = installationNode->getModules();
				for(auto module : modules) {
					auto result = Solvers::Reworld::Navigate::PointToPoint::solve(module->getModel()
						, module->getCurrentAxisAngles()
						, point1
						, point2
						, this->parameters.solverSettings.getSolverSettings());

					module->setTargetAxisAngles(result.solution.axisAngles);
					cout << result.residual << endl;
				}
			}
		}
	}
}
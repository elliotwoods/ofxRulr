#include "pch_RulrNodes.h"
#include "Startup.h"

namespace ofxRulr {
	namespace Nodes {
		namespace Watchdog {
			//----------
			Startup::Startup() {
				RULR_NODE_INIT_LISTENER;
			}

			//----------
			string Startup::getTypeName() const {
				return "Watchdog::Startup";
			}

			//----------
			void Startup::init() {
				RULR_NODE_UPDATE_LISTENER;
				RULR_NODE_INSPECTOR_LISTENER;

				this->manageParameters(this->parameters);

				this->addInput<Nodes::Base>();
			}

			//----------
			void Startup::update() {
				if (ofGetFrameNum() == this->parameters.triggerOnFrameNumber || this->forceSimulate) {
					if (this->parameters.inspectNode.enabled) {
						auto node = this->getInput<Nodes::Base>();
						if (node) {
							ofxCvGui::InspectController::X().inspect(node);
						}
					}

					if (this->parameters.inspectNode.maximiseInspector) {
						ofxCvGui::InspectController::X().maximise();
					}

					if (this->parameters.reshapeWindow.enabled) {
						const auto & bounds = this->parameters.reshapeWindow.bounds.get();
						ofSetWindowPosition(bounds.x, bounds.y);
						ofSetWindowShape(bounds.width, bounds.height);
					}

					this->forceSimulate = false;
				}
			}

			//----------
			void Startup::populateInspector(ofxCvGui::InspectArguments & inspectArgs) {
				auto inspector = inspectArgs.inspector;
				inspector->addButton("Simulate start", [this]() {
					this->forceSimulate = true;
				}, OF_KEY_RETURN)->setHeight(100.0f);

				inspector->addButton("Capture bounds", [this]() {
					this->parameters.reshapeWindow.bounds = ofGetWindowRect();
				});
			}
		}
	}
}
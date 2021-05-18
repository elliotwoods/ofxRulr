#include "pch_RulrNodes.h"
#include "Debugger.h"
#include "ofxCvGui.h"

namespace ofxRulr {
	namespace Nodes {
		namespace Application {
			//----------
			Debugger::Debugger() {
				RULR_NODE_INIT_LISTENER;
			}

			//----------
			string Debugger::getTypeName() const {
				return "Application::Debugger";
			}

			//----------
			void Debugger::init() {
				this->manageParameters(ofxCvGui::Utils::Debugger::X().parameters);
			}
		}
	}
}

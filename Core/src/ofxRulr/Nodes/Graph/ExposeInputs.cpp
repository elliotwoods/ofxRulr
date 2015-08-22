#include "ExposeInputs.h"

namespace ofxRulr {
	namespace Nodes {
		namespace Graph {
			//--------
			ExposeInputs::ExposeInputs() {
				RULR_NODE_INIT_LISTENER;
			}

			//--------
			string ExposeInputs::getTypeName() const {
				return "Graph::ExposeInputs";
			}

			//--------
			void ExposeInputs::init() {

			}

			//--------
			void ExposeInputs::setHost(shared_ptr<Patch> host) {
				this->host = host;
			}
		}
	}
}
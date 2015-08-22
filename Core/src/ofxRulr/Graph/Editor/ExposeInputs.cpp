#include "ExposeInputs.h"

namespace ofxRulr {
	namespace Graph {
		namespace Editor {
			//--------
			ExposeInputs::ExposeInputs() {
			}

			//--------
			void ExposeInputs::init() {

			}

			//--------
			string ExposeInputs::getTypeName() const {
				return "Graph::ExposeInputs";
			}

			//--------
			void ExposeInputs::setHostPatch(shared_ptr<Patch> host) {
				this->host = host;
			}
		}
	}
}
#include "NodeHost.h"

namespace ofxDigitalEmulsion {
	namespace Graph {
		namespace Patch {
			//----------
			NodeHost::NodeHost() {

			}

			//----------
			shared_ptr<Node> NodeHost::getNodeInstance() {
				return this->node;
			}
		}
	}
}
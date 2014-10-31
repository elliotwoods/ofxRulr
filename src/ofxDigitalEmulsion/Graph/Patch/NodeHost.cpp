#include "NodeHost.h"

namespace ofxDigitalEmulsion {
	namespace Graph {
		namespace Patch {
			//----------
			NodeHost::NodeHost(shared_ptr<Node> node) {
				this->node = node;
				this->setBounds(ofRectangle(200, 200, 200, 200));

				this->onDraw += [this](ofxCvGui::DrawArguments & args) {
					ofxCvGui::Utils::drawText(this->getNodeInstance()->getTypeName(), this->getLocalBounds());
				};
			}

			//----------
			shared_ptr<Node> NodeHost::getNodeInstance() {
				return this->node;
			}
		}
	}
}
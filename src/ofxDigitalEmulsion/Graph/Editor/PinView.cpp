#include "PinView.h"
#include "ofxAssets.h"

namespace ofxDigitalEmulsion {
	namespace Graph {
		namespace Editor {
			//----------
			PinView::PinView(string nodeTypeName) {
				this->nodeTypeName = nodeTypeName;

				this->onDraw += [this](ofxCvGui::DrawArguments & args) {
					ofxAssets::image("ofxDigitalEmulsion::Nodes::" + this->nodeTypeName).draw(args.localBounds);
					if (this->isMouseOver()) {
						ofxCvGui::Utils::drawToolTip(this->nodeTypeName, ofVec2f(args.localBounds.getWidth() / 2.0f, 0.0f));
					};
				};
			}

			//----------
			void PinView::setTypeName(string nodeTypeName) {
				this->nodeTypeName = nodeTypeName;
			}
		}
	}
}
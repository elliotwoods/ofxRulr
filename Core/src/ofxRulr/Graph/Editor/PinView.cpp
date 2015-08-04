#include "PinView.h"
#include "ofxAssets.h"
#include "../../Nodes/Base.h"

namespace ofxRulr {
	namespace Graph {
		namespace Editor {
			//----------
			PinView::PinView() {
				this->onDraw += [this](ofxCvGui::DrawArguments & args) {
					if (this->icon) {
						this->icon->draw(args.localBounds);
					}
					if (this->isMouseOver()) {
						ofxCvGui::Utils::drawToolTip(this->nodeTypeName, ofVec2f(args.localBounds.getWidth() / 2.0f, 0.0f));
					};
				};
			}

			//---------
			void PinView::setup(Nodes::Base & node) {
				this->icon = node.getIcon();
				this->nodeTypeName = node.getTypeName();
			}
		}
	}
}
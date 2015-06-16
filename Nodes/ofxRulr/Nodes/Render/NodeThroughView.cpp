#include "NodeThroughView.h"

#include "../Item/View.h"
#include "../Device/VideoOutput.h"

using namespace ofxRulr::Nodes;

namespace ofxRulr {
	namespace Render {
		//----------
		NodeThroughView::NodeThroughView() {
			RULR_NODE_INIT_LISTENER;
		}

		//----------
		string NodeThroughView::getTypeName() const {
			return "Render::NodeThroughView";
		}

		//----------
		void NodeThroughView::init() {
			this->addInput<Node>();
			this->addInput<Item::View>();
			auto videoOutputInput = this->addInput<Device::VideoOutput>();

			videoOutputInput->onNewConnection += [this](shared_ptr<Device::VideoOutput> videoOutput) {
				videoOutput->onDrawOutput.addListener([this](ofRectangle & videoBounds) {
					this->drawOnVideoOutput(videoBounds);
				}, this);
			};
			videoOutputInput->onDeleteConnection += [this](shared_ptr<Device::VideoOutput> videoOutput) {
				if (videoOutput) {
					videoOutput->onDrawOutput.removeListeners(this);
				}
			};
		}

		//----------
		void NodeThroughView::drawOnVideoOutput(const ofRectangle & viewBounds) {
			auto node = this->getInput<Node>();
			auto view = this->getInput<Item::View>();

			if (node && view) {
				view->getViewInWorldSpace().beginAsCamera(true);
				node->drawWorld();
				view->getViewInWorldSpace().endAsCamera();
			}
		}
	}
}
#include "pch_RulrNodes.h"
#include "VideoOutputListener.h"

namespace ofxRulr {
	namespace Utils {
		//----------
		VideoOutputListener::VideoOutputListener(shared_ptr<Graph::Pin<Nodes::System::VideoOutput>> pin, function<void(const ofRectangle &)> action)
		: pin(pin)
		, action(action) {
			pin->onNewConnection += [this](shared_ptr<Nodes::System::VideoOutput> node) {
				node->onDrawOutput.addListener([this](ofRectangle & bounds) {
					this->action(bounds);
				}, this);
			};

			pin->onDeleteConnection += [this](shared_ptr<Nodes::System::VideoOutput> node) {
				if (node) {
					node->onDrawOutput.removeListeners(this);
				}
			};
		}

		//----------
		VideoOutputListener::~VideoOutputListener() {
			auto node = this->pin->getConnection();
			if (node) {
				node->onDrawOutput.removeListeners(this);
			}
		}
	}
}
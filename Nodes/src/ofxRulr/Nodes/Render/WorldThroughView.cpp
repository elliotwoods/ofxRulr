#include "pch_RulrNodes.h"
#include "WorldThroughView.h"

#include "ofxRulr/Nodes/Item/View.h"
#include "ofxRulr/Nodes/System/VideoOutput.h"

using namespace ofxRulr::Nodes;

namespace ofxRulr {
	namespace Nodes {
		namespace Render {
			//----------
			WorldThroughView::WorldThroughView() {
				RULR_NODE_INIT_LISTENER;
			}

			//----------
			string WorldThroughView::getTypeName() const {
				return "Render::WorldThroughView";
			}

			//----------
			void WorldThroughView::init() {
				this->addInput<Item::View>();
				auto videoOutputInput = this->addInput<System::VideoOutput>();

				videoOutputInput->onNewConnection += [this](shared_ptr<System::VideoOutput> videoOutput) {
					videoOutput->onDrawOutput.addListener([this](ofRectangle& videoBounds) {
						this->drawOnVideoOutput(videoBounds);
						}, this);
				};
				videoOutputInput->onDeleteConnection += [this](shared_ptr<System::VideoOutput> videoOutput) {
					if (videoOutput) {
						videoOutput->onDrawOutput.removeListeners(this);
					}
				};
			}

			//----------
			void WorldThroughView::drawOnVideoOutput(const ofRectangle& viewBounds) {
				auto view = this->getInput<Item::View>();

				static auto& world = ofxRulr::Graph::World::X();

				if (view) {
					view->getViewInWorldSpace().beginAsCamera(true);
					{
						DrawWorldAdvancedArgs args;
						args.enableGui = false;
						world.drawWorldAdvanced(args);
					}
					view->getViewInWorldSpace().endAsCamera();
				}
			}
		}
	}
}
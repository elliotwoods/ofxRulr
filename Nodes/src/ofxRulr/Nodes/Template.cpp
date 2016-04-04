#include "pch_RulrNodes.h"

#include "Template.h"

#include "ofxRulr/Nodes/Item/Camera.h"

// See https://rulr.hackpad.com/Nodes::Template for notes

namespace ofxRulr {
	namespace Nodes {
		//----------
		Template::Template() {
			// We keep our constructor as empty as possible.
			// The constructor can be called at any time (even when no instance of the node exists).

			// Add a listener to onInit, to call our local init() function where we perform our heavy stuff.
			RULR_NODE_INIT_LISTENER;
		}

		//----------
		string Template::getTypeName() const {
			// Return a string with our type name.
			// Remember to include the namespace beneath ofxRulr::Nodes, e.g.
			//  if your node is `ofxRulr::Nodes::Item::Camera`, then here you
			//  should return `Item::Camera`
			return "Template";
		}

		//----------
		ofxCvGui::PanelPtr Template::getPanel() {
			return this->panel;
		}

		//----------
		void Template::init() {
			// This function is called once when our node is created in the Patch

			// Generally the first thing we do is add listeners for our other usual activities
			RULR_NODE_UPDATE_LISTENER; // activate update()

			this->addInput<Nodes::Item::Camera>();

			this->panel = ofxCvGui::Panels::makeImage(this->image, "Inverted");
		}

		//----------
		void Template::update() {
			// Get a shared pointer to whatever is attached to this input.
			// The pointer will be empty is nothing is attached (or you asked
			// for an input which doesn't exist).
			auto cameraNode = this->getInput<Item::Camera>();

			if (cameraNode) {
				this->image = cameraNode->getGrabber()->getPixels();

				for (auto & pixel : image.getPixels()) {
					pixel = 255 - pixel;
				}

				this->image.update();
			}
		}
	}
}
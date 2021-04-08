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
			RULR_NODE_SERIALIZATION_LISTENERS;
			RULR_NODE_INSPECTOR_LISTENER;

			this->addInput<Nodes::Item::Camera>("Camera 1");

			auto panel = ofxCvGui::Panels::makeImage(this->image, "Inverted");
			panel->onDraw += [](ofxCvGui::DrawArguments & args) {
				ofDrawBitmapStringHighlight("drawing in panel space", 20, 100);
			};
			panel->onDrawImage += [](ofxCvGui::DrawImageArguments& args) {
				ofDrawBitmapStringHighlight("drawing in image space", 500, 100);
			};
			this->panel = panel;
		}

		//----------
		void Template::update() {
			// Get a shared pointer to whatever is attached to this input.
			// The pointer will be empty is nothing is attached (or you asked
			// for an input which doesn't exist).
			auto cameraNode = this->getInput<Item::Camera>();

			// First check it's not empty (i.e. make sure something is attached)
			if (cameraNode) {

				// Some simple code to create a local inverted image
				this->image = cameraNode->getGrabber()->getPixels();

				for (auto & pixel : image.getPixels()) {
					pixel = 255 - pixel;
				}

				this->image.update();
			}
		}

		//----------
		void Template::serialize(nlohmann::json & json) {
			// This code is run whenever we save.
			// `nlohmann::json & json` is our own personal json value to write to

			// We can use a utility function to do the writing for us
			Utils::serialize(json, this->parameters);

			// Or we can write manually
			json["something"] = 10;
		}

		//----------
		void Template::deserialize(const nlohmann::json & json) {
			// This code is run whenever we load.
			// `const nlohmann::json & json` is our own personal json value to read from

			// We can use a utility function to do the reading for us
			Utils::deserialize(json, this->parameters);

			// Or we can read manually (note the pattern when deserialising - in case the value doesn't exist
			int something;
			ofxRulr::Utils::deserialize(json, "something", something);
		}

		//----------
		void Template::populateInspector(ofxCvGui::InspectArguments & args) {
			// This function is called when we are selected in the Patch
			// It allows us to fill out the widgets on the right hand Inspector panel

			// First get the inspector itself from the arguments
			auto inspector = args.inspector;
			// This inherits type ofxCvGui::Panels::Widgets, so it's easy to add widgets to

			// This utility function will add parameters from a group (and its subgroups) to the interface
			inspector->addParameterGroup(this->parameters);
		}
	}
}
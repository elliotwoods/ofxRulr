#pragma once

#include "ofxRulr/Nodes/Base.h"

// See https://rulr.hackpad.com/Nodes::Template for notes

//all nodes are inside ofxRulr::Nodes
namespace ofxRulr {
	namespace Nodes {

		//usually we put our node class into a suitable namespace

		class Template : public Nodes::Base {
		public:
			// Called often and without your control
			Template();

			// Every node must implement getTypeName()
			string getTypeName() const override;

			ofxCvGui::PanelPtr getPanel() override;
		private:
			// Called when Node is added to Patch
			void init();

			// Called once every frame
			void update();

			// The panel view. Sometimes we might want to store the shared_ptr to the specific type
			ofxCvGui::PanelPtr panel;

			// A local image asset used in this example
			ofImage image;
		};
	}
}
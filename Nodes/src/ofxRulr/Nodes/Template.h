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

			void serialize(Json::Value &);
			void deserialize(const Json::Value &);
			void populateInspector(ofxCvGui::InspectArguments &);
		private:
			// Called when Node is added to Patch
			void init();

			// Called once every frame
			void update();

			// The panel view. Sometimes we might want to store the shared_ptr to the specific type
			ofxCvGui::PanelPtr panel;

			// A local image asset used in this example
			ofImage image;



			// A group of parameters using our special sauce
			struct : public ofParameterGroup {
				// A parameter inside the group using list initialisation to set defaults
				ofParameter<float> myValue{ "My Value", 0, 0, 10 };

				// A subgroup (using the same syntax again)
				struct : public ofParameterGroup {
					ofParameter<float> mySubValue{ "My sub value", 0, 0, 10 };

					PARAM_DECLARE("subParameters", mySubValue);
				} subParameters;

				// For our special sauce to work, we must use the PARAM_DECLARE macro.
				// It takes the name of the group, and the parameters/groups inside it
				PARAM_DECLARE("Template", myValue, subParameters);
				// Note the top level group generally takes the name of this class
				
			} parameters; // <- notice that the struct doesn't need a typename (e.g. Parameters), but it does need an instance name (e.g. parameters)
		};
	}
}
#pragma once

#include "ofxRulr/Nodes/Base.h"

namespace ofxRulr {
	namespace Nodes {
		namespace Export {
			class Mesh : public Nodes::Base {
			public:
				Mesh();
				string getTypeName() const override;

				void init();

				void populateInspector(ofxCvGui::InspectArguments &);

				void exportOBJ();
				void exportPLY();
			};
		}
	}
}
#pragma once

#include "Style.h"
#include "ofxRulr/Nodes/Item/RigidBody.h"

namespace ofxRulr {
	namespace Nodes {
		namespace Render {
			class Draw : public Style, public Item::RigidBody {
			public:
				Draw();
				string getTypeName() const override;
				void init();
			protected:
				void populateInspector(ofxCvGui::InspectArguments &);
				void customBegin() override;
				void customEnd() override;

				struct : ofParameterGroup {
					ofParameter<int> culling{ "Culling", 0 };
					ofParameter<int> frontFace{ "Front face", 0};

					PARAM_DECLARE("Drawing", culling, frontFace);
				} parameters;
			};
		}
	}
}
#pragma once

#include "Style.h"
#include "ofxRulr/Nodes/Item/RigidBody.h"

namespace ofxRulr {
	namespace Nodes {
		namespace Render {
			class Draw : public Style, public Item::RigidBody {
			public:
				MAKE_ENUM(Culling
					, (Back, Front, FrontAndBack)
					, ("Back", "Front", "Front+Back"));

				MAKE_ENUM(FrontFace
					, (CCW, CW)
					, ("CCW", "CW"));

				Draw();
				string getTypeName() const override;
				void init();
			protected:
				void populateInspector(ofxCvGui::InspectArguments &);
				void customBegin() override;
				void customEnd() override;

				struct : ofParameterGroup {
					ofParameter<Culling> culling{ "Culling", Culling::Back };
					ofParameter<FrontFace> frontFace{ "Front face", FrontFace::CCW };

					PARAM_DECLARE("Drawing", culling, frontFace);
				} parameters;
			};
		}
	}
}
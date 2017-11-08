#pragma once

#include "Style.h"
#include "ofxRulr/Nodes/Item/RigidBody.h"

namespace ofxRulr {
	namespace Nodes {
		namespace Render {
			class Lighting : public Style, public Item::RigidBody {
			public:
				MAKE_ENUM(LightMode
					, (Point, Spot, Directional)
					, ("Point", "Spot", "Directional"));
				Lighting();
				string getTypeName() const override;
				void init();
			protected:
				void customBegin() override;
				void customEnd() override;

				struct : ofParameterGroup {
					ofParameter<LightMode> lightMode;

					struct : ofParameterGroup {
						ofParameter<float> cutoffAngle{ "Cutoff angle", 45 };
						PARAM_DECLARE("Spotlight", cutoffAngle);
					} spotLight;

					struct : ofParameterGroup {
						ofParameter<ofFloatColor> ambient{ "Ambient color", ofColor(0) };
						ofParameter<ofFloatColor> diffuse{ "Diffuse color", ofColor(100) };
						ofParameter<ofFloatColor> specular{ "Specular color", ofColor(255) };
						PARAM_DECLARE("Color", ambient, diffuse, specular);
					} color;

					PARAM_DECLARE("Lighting", lightMode, spotLight, color);
				} parameters;

				ofLight light;
			};
		}
	}
}
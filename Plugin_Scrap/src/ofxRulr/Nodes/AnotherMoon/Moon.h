#pragma once

#include "ofxRulr.h"
#include "ofxRulr/Nodes/Item/RigidBody.h"

namespace ofxRulr {
	namespace Nodes {
		namespace AnotherMoon {
			class Moon : public Nodes::Item::RigidBody {
			public:
				Moon();
				string getTypeName() const override;

				void init();
				void drawObject();

				float getRadius() const;
			protected:
				struct : ofParameterGroup {
					ofParameter<float> diameter{ "Diameter", 5.0f };

					struct : ofParameterGroup {
						ofParameter<int> resolution{ "Resolution", 5 };
						ofParameter<WhenActive> filled{ "Filled", WhenActive::Selected };
						PARAM_DECLARE("Draw", resolution, filled);
					} draw;

					PARAM_DECLARE("Moon", diameter, draw);
				} parameters;
			};
		}
	}
}
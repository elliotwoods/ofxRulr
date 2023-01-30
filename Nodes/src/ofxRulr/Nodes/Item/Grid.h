#pragma once

#include "RigidBody.h"

namespace ofxRulr {
	namespace Nodes {
		namespace Item {
			class Grid : public RigidBody {
			public:
				Grid();
				void init();
				void update();

				string getTypeName() const override;

				void drawObject();
			protected:
				void buildMesh();
				ofMesh previewMesh;

				struct Parameters : ofParameterGroup {
					ofParameter<int> countX{ "Squares X", 10, 2, 1000};
					ofParameter<int> countY{ "Squares Y", 10, 2, 1000};
					ofParameter<float> spacing{ "Spacing [m]", 1.0f, 0.001f, 100.0f };
					ofParameter<bool> crosses{ "Crosses", false };
					PARAM_DECLARE("Grid", countX, countY, spacing, crosses);
				};

				Parameters parameters;
				Parameters cachedParameters;

				bool meshStale = true;
				ofMesh mesh;
			};
		}
	}
}
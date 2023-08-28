#pragma once

#include "ofxRulr/Nodes/IHasVertices.h"
#include "ofxRulr/Data/Reworld/Column.h"

namespace ofxRulr {
	namespace Nodes {
		namespace Reworld {
			class Installation : public IHasVertices, public Item::RigidBody {
			public:
				Installation();
				string getTypeName() const override;

				void init();
				void update();
				void drawObjectAdvanced(DrawWorldAdvancedArgs&);

				void serialize(nlohmann::json&);
				void deserialize(const nlohmann::json&);
				void populateInspector(ofxCvGui::InspectArguments&);

				void build();

				vector<glm::vec3> getVertices() const override;
			protected:
				struct : ofParameterGroup {
					struct : ofParameterGroup {
						Data::Reworld::Panel::BuildParameters panel;
						Data::Reworld::Column::BuildParameters column;

						struct : ofParameterGroup {
							ofParameter<int> countX{ "Count X", 22 };
							ofParameter<float> horizontalStride{ "Horizontal stride", 0.48, 0, 1 };
							ofParameter<float> angleBetweenColumns{ "Angle between columns [deg]", 3.913043478, 0, 20 };
							PARAM_DECLARE("Installation", countX, horizontalStride, angleBetweenColumns);
						} installation;

						PARAM_DECLARE("Builder", panel, column, installation);
					} builder;
					PARAM_DECLARE("Installation", builder);
				} parameters;

				Utils::CaptureSet<Data::Reworld::Column> columns;
			};
		}
	}
}
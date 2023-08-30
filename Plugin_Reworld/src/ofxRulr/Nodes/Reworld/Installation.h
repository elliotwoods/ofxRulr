#pragma once

#include "ofxRulr/Nodes/IHasVertices.h"
#include "ofxRulr/Data/Reworld/Column.h"
#include "ofxRulr/Utils/EditSelection.h"

namespace ofxRulr {
	namespace Nodes {
		namespace Reworld {
			class Installation : public IHasVertices
			{
			public:
				struct LabelDraws : ofParameterGroup {
					ofParameter<WhenActive> column{ "Column", WhenActive::Always };
					ofParameter<WhenActive> panel{ "Panel", WhenActive::Selected };
					ofParameter<WhenActive> portal{ "Portal", WhenActive::Selected };
					PARAM_DECLARE("Labels", column, panel, portal);
				};

				Installation();
				string getTypeName() const override;

				void init();
				void update();
				void drawObjectAdvanced(DrawWorldAdvancedArgs&);

				void serialize(nlohmann::json&);
				void deserialize(const nlohmann::json&);
				void populateInspector(ofxCvGui::InspectArguments&);
				ofxCvGui::PanelPtr getPanel() override;

				void build();
				void initColumns();

				vector<glm::vec3> getVertices() const override;

				Utils::EditSelection<Data::Reworld::Column> ourSelection;
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

					struct : ofParameterGroup {
						LabelDraws labels;
						PARAM_DECLARE("Draw", labels);
					} draw;

					PARAM_DECLARE("Installation", builder, draw);
				} parameters;

				shared_ptr<ofxCvGui::Panels::Widgets> panel;
				shared_ptr<Nodes::Item::RigidBody> rigidBody;

				Utils::CaptureSet<Data::Reworld::Column> columns;
			};
		}
	}
}
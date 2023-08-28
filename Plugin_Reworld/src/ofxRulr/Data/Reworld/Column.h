#pragma once

#include "Panel.h"
#include "ofxRulr/Utils/CaptureSet.h"
#include "ofxRulr/Nodes/Item/RigidBody.h"
#include "ofxRulr/Nodes/IHasVertices.h"

namespace ofxRulr {
	namespace Data {
		namespace Reworld {
			class Column
				: public Utils::AbstractCaptureSet::BaseCapture
				, ofxCvGui::IInspectable
			{
			public:
				struct BuildParameters : ofParameterGroup {
					ofParameter<int> countY{ "Count Y", 5 };
					ofParameter<float> yStart{ "Y Start", -1, -10, 10 };
					ofParameter<float> verticalStride{ "Vertical stride", -0.45, -1.0, 1.0 };
					PARAM_DECLARE("Column", countY, yStart, verticalStride);
				};

				Column();
				string getTypeName() const override;
				string getDisplayString() const override;

				void drawWorldAdvanced(DrawWorldAdvancedArgs&);
				void drawObjectAdvanced(DrawWorldAdvancedArgs&);
				vector<glm::vec3> getVertices() const;

				void init();
				void serialize(nlohmann::json&);
				void deserialize(const nlohmann::json&);

				void build(const BuildParameters&, const Panel::BuildParameters&, int columnIndex);

				struct : ofParameterGroup {
					ofParameter<int> index{ "Index", 0 };
					PARAM_DECLARE("Column", index);
				} parameters;

				shared_ptr<Nodes::Item::RigidBody> rigidBody;

				Utils::CaptureSet<Panel> panels;

				Utils::EditSelection<Panel> ourSelection;
				Utils::EditSelection<Column>* parentSelection = nullptr;

			protected:
				ofxCvGui::ElementPtr getDataDisplay() override;
			};
		}
	}
}
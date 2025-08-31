#pragma once

#include "Portal.h"
#include "ofxRulr/Utils/CaptureSet.h"
#include "ofxRulr/Nodes/Item/RigidBody.h"
#include "ofxRulr/Nodes/IHasVertices.h"

namespace ofxRulr {
	namespace Data {
		namespace Reworld {
			class Panel
				: public Utils::AbstractCaptureSet::BaseCapture
				, ofxCvGui::IInspectable
			{
			public:
				struct BuildParameters : ofParameterGroup {
					ofParameter<float> pitch{ "Pitch", 0.15f, 0.0f, 1.0f };
					ofParameter<int> countX{ "Count X", 3 };
					ofParameter<int> countY{ "Count Y", 3 };
					PARAM_DECLARE("Panel", pitch, countX, countY);
				};

				Panel();
				string getTypeName() const override;
				string getDisplayString() const override;

				void drawWorldAdvanced(DrawWorldAdvancedArgs&);
				void drawObjectAdvanced(DrawWorldAdvancedArgs&);
				vector<glm::vec3> getVertices() const;

				void init();
				void serialize(nlohmann::json&);
				void deserialize(const nlohmann::json&);

				void build(const BuildParameters&, int targetIndexOffset);

				struct : ofParameterGroup {
					ofParameter<float> width{ "Width", 0.15f * 3, 0, 1 };
					ofParameter<float> height{ "Height", 0.15f * 3, 0, 1 };
					PARAM_DECLARE("Panel", width, height);
				} parameters;

				shared_ptr<Nodes::Item::RigidBody> rigidBody;

				Utils::CaptureSet<Portal> portals;

				Utils::EditSelection<Portal> ourSelection;
				Utils::EditSelection<Panel>* parentSelection = nullptr;

			protected:
				ofxCvGui::ElementPtr getDataDisplay() override;
			};
		}
	}
}
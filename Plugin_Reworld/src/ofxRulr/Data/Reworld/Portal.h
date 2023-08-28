#pragma once

#include "ofxRulr.h"
#include "ofxRulr/Nodes/Item/RigidBody.h"
#include "ofxRulr/Nodes/IHasVertices.h"

#define REWORLD_PORTAL_SHROUD_SIZE 0.1286

namespace ofxRulr {
	namespace Data {
		namespace Reworld {
			class Portal
				: public Utils::AbstractCaptureSet::BaseCapture
				, ofxCvGui::IInspectable
			{
			public:
				Portal();

				string getTypeName() const override;
				string getDisplayString() const override;

				void build(int targetIndex);

				void drawWorldAdvanced(DrawWorldAdvancedArgs&);
				void drawObjectAdvanced(DrawWorldAdvancedArgs&);

				vector<glm::vec3> getVertices() const;

				void init();
				void serialize(nlohmann::json&);
				void deserialize(const nlohmann::json&);

				struct : ofParameterGroup {
					ofParameter<int> target{ "Target", 1 };
					PARAM_DECLARE("Portal", target);
				} parameters;

				static shared_ptr<ofTexture> panelPreview;

				shared_ptr<Nodes::Item::RigidBody> rigidBody;

				Utils::EditSelection<Portal>* parentSelection = nullptr;

			protected:
				ofxCvGui::ElementPtr getDataDisplay() override;
			};
		}
	}
}
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
				, public Nodes::Item::RigidBody
				, public Nodes::IHasVertices 
			{
			public:
				Portal();

				string getTypeName() const override;
				string getDisplayString() const override;

				void build(int targetIndex);

				void drawObjectAdvanced(DrawWorldAdvancedArgs&);
				vector<glm::vec3> getVertices() const override;

				struct : ofParameterGroup {
					ofParameter<int> target{ "Target", 1 };
					PARAM_DECLARE("Portal", target);
				} parameters;

				static shared_ptr<ofTexture> panelPreview;
			};
		}
	}
}
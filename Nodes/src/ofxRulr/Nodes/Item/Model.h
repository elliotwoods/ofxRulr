#pragma once

#include "Base.h"

#include <ofxCvGui/Panels/World.h>

class ofxAssimpModelLoader;

namespace ofxRulr {
	namespace Nodes {
		namespace Item {
			class Model : public Base {
			public:
				Model();
				string getTypeName() const override;
				void init();
				ofxCvGui::PanelPtr getPanel() override;

				void update();
				void drawWorld();

				void serialize(Json::Value &);
				void deserialize(const Json::Value &);
			protected:
				void populateInspector(ofxCvGui::InspectArguments &);

				void updatePreviewMesh();

				shared_ptr<ofxCvGui::Panels::World> view;

				ofParameter<string> filename;

				ofParameter<bool> drawVertices, drawWireframe, drawFaces;

				ofParameter<bool> flipX, flipY, flipZ;
				ofParameter<float> inputUnitScale;

				shared_ptr<ofxAssimpModelLoader> modelLoader;
			};
		}
	}
}
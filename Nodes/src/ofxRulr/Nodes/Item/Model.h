#pragma once

#include "Base.h"

#include <ofxCvGui/Panels/World.h>
#include "ofxAssimpModelLoader.h"

namespace ofxRulr {
	namespace Nodes {
		namespace Item {
			class Model : public Base {
			public:
				MAKE_ENUM(Axes
					, (NegX, PosX, NegY, PosY, NegZ, PosZ)
					, ("-X", "+X", "-Y", "+Y", "-Z", "+Z"));

				Model();
				string getTypeName() const override;
				void init();
				void update();
				void drawWorld();

				void serialize(Json::Value &);
				void deserialize(const Json::Value &);

				ofMatrix4x4 getMeshTransform() const;
			protected:
				void populateInspector(ofxCvGui::InspectArguments &);

				void updatePreviewMesh();

				ofParameter<string> filename;

				unique_ptr<ofxAssimpModelLoader> modelLoader;

				struct : ofParameterGroup {
					struct : ofParameterGroup {
						ofParameter<bool> vertices{ "Vertices", false };
						ofParameter<bool> wireframe{ "Wireframe", false };
						ofParameter<bool> faces{ "Faces", true };
						PARAM_DECLARE("Draw style", vertices, wireframe, faces);
					} drawStyle;

					struct : ofParameterGroup {
						ofParameter<float> scale{ "Scale", 1.0f, 0.0f, 1000.0f };
						ofParameter<Axes> theirXIsOur{ "Their X is our", Axes::PosX };
						ofParameter<Axes> theirYIsOur{ "Their Y is our", Axes::PosY };
						ofParameter<Axes> theirZIsOur{ "Their Z is our", Axes::PosZ };
						PARAM_DECLARE("Transform", scale, theirXIsOur, theirYIsOur, theirZIsOur);
					} transform;

					PARAM_DECLARE("Model", drawStyle, transform);
				} parameters;
			};
		}
	}
}
#pragma once

#include "RigidBody.h"

#include <ofxCvGui/Panels/World.h>
#include "ofxAssimpModelLoader.h"

#include "ofxRulr/Nodes/IHasVertices.h"

namespace ofxRulr {
	namespace Nodes {
		namespace Item {
			class Mesh : public RigidBody, public IHasVertices {
			public:
				MAKE_ENUM(Axes
					, (NegX, PosX, NegY, PosY, NegZ, PosZ)
					, ("-X", "+X", "-Y", "+Y", "-Z", "+Z"));

				MAKE_ENUM(Cull
					, (None, CW, CCW)
					, ("None", "CW", "CCW"));

				Mesh();
				string getTypeName() const override;
				void init();
				void update();
				void drawObject();
				void drawObjectAdvanced(DrawWorldAdvancedArgs &);

				void serialize(nlohmann::json &);
				void deserialize(const nlohmann::json &);

				ofMatrix4x4 getMeshTransform() const;

				virtual vector<glm::vec3> getVertices() const override;

			protected:
				void populateInspector(ofxCvGui::InspectArguments &);
				void loadMesh();
				void loadTexture();

				bool meshDirty = true;
				bool textureDirty = true;

				ofParameter<string> meshFilename{ "Mesh filename", "" };
				ofParameter<string> textureFilename{ "Texture filename", "" };

				unique_ptr<ofxAssimpModelLoader> modelLoader;
				ofImage texture;

				struct : ofParameterGroup {
					struct : ofParameterGroup {
						ofParameter<bool> vertices{ "Vertices", false };
						ofParameter<bool> wireframe{ "Wireframe", false };
						ofParameter<bool> faces{ "Faces", true };
						ofParameter<Cull> cull{ "Cull", Cull::None };
						ofParameter<ofFloatColor> color{ "Color", ofColor(1.0f) };
						PARAM_DECLARE("Draw style", vertices, wireframe, faces, cull, color);
					} drawStyle;

					struct : ofParameterGroup {
						ofParameter<float> scale{ "Scale", 1.0f, 0.0f, 1000.0f };
						ofParameter<Axes> theirXIsOur{ "Their X is our", Axes::PosX };
						ofParameter<Axes> theirYIsOur{ "Their Y is our", Axes::PosY };
						ofParameter<Axes> theirZIsOur{ "Their Z is our", Axes::PosZ };
						PARAM_DECLARE("Transform", scale, theirXIsOur, theirYIsOur, theirZIsOur);
					} transform;

					struct : ofParameterGroup {
						ofParameter<bool> enable{ "Enable", true };
						ofParameter<bool> normalizeCoordinates{ "Normalize coordinates", true };
						PARAM_DECLARE("Texture", enable, normalizeCoordinates);
					} texture;

					PARAM_DECLARE("Model", drawStyle, transform);
				} parameters;

				struct {
					size_t numMeshes = 0;
					string vertexCount;
					glm::vec3 sceneMin;
					glm::vec3 sceneMax;
					bool needsRecalculate = true;
				} meshAnalysisCache;
			};
		}
	}
}
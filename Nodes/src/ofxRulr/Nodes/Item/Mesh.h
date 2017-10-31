#pragma once

#include "RigidBody.h"

#include <ofxCvGui/Panels/World.h>
#include "ofxAssimpModelLoader.h"

namespace ofxRulr {
	namespace Nodes {
		namespace Item {
			class Mesh : public RigidBody {
			public:
				MAKE_ENUM(Axes
					, (NegX, PosX, NegY, PosY, NegZ, PosZ)
					, ("-X", "+X", "-Y", "+Y", "-Z", "+Z"));

				Mesh();
				string getTypeName() const override;
				void init();
				void update();
				void drawObject();

				void serialize(Json::Value &);
				void deserialize(const Json::Value &);

				ofMatrix4x4 getMeshTransform() const;

				ofVec3f getVertexCloseToWorldPosition(const ofVec3f &) const;
				ofVec3f getVertexCloseToMouse(float maxDistance = 30.0f) const;
				ofVec3f getVertexCloseToMouse(const ofVec3f & mousePosition, float maxDistance = 30.0f) const;
			protected:
				void populateInspector(ofxCvGui::InspectArguments &);

				ofParameter<string> filename;

				unique_ptr<ofxAssimpModelLoader> modelLoader;

				struct : ofParameterGroup {
					struct : ofParameterGroup {
						ofParameter<bool> vertices{ "Vertices", false };
						ofParameter<bool> wireframe{ "Wireframe", false };
						ofParameter<bool> faces{ "Faces", false };
						ofParameter<ofFloatColor> color{ "Color", ofColor(1.0f) };
						PARAM_DECLARE("Draw style", vertices, wireframe, faces, color);
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
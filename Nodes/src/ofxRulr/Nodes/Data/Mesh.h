#pragma once

#include "ofxRulr/Nodes/Base.h"

namespace ofxRulr {
	namespace Nodes {
		namespace Data {
			class Mesh : public Gives<ofMesh> {
			public:
				Mesh();
				string getTypeName() const override;


				void init();
				void serialize(Json::Value &);
				void deserialize(const Json::Value &);
				void populateInspector(ofxCvGui::InspectArguments &);

				void get(ofMesh &) const override;
				ofMesh & getMesh();
				const ofMesh & getMesh() const;

				void save(string filePath = "") const;
				void load(string filePath = "");

			protected:
				ofMesh mesh;
			};
		}
	}
}
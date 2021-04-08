#pragma once

#include "ofxRulr/Nodes/Base.h"

namespace ofxRulr {
	namespace Nodes {
		namespace Data {
			class Mesh : public Nodes::Base {
			public:
				Mesh();
				string getTypeName() const override;


				void init();
				void serialize(nlohmann::json &);
				void deserialize(const nlohmann::json &);
				void populateInspector(ofxCvGui::InspectArguments &);

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
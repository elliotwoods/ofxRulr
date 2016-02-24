#include "pch_RulrNodes.h"
#include "Mesh.h"

namespace ofxRulr {
	namespace Nodes {
		namespace Export {
			//----------
			Mesh::Mesh() {
				RULR_NODE_INIT_LISTENER;
			}

			//----------
			string Mesh::getTypeName() const {
				return "Export::Mesh";
			}

			//----------
			void Mesh::init() {
				RULR_NODE_INSPECTOR_LISTENER;

				this->addInput<Gives<ofMesh>>();
			}

			//----------
			void Mesh::populateInspector(ofxCvGui::InspectArguments & args) {
				auto inspector = args.inspector;

				inspector->addButton("Export OBJ...", [this]() {
					try {
						this->exportOBJ();
					}
					RULR_CATCH_ALL_TO_ALERT;
				});

				inspector->addButton("Export PLY...", [this]() {
					try {
						this->exportPLY();
					}
					RULR_CATCH_ALL_TO_ALERT;
				});
			}

			//----------
			void Mesh::exportOBJ() {
				this->throwIfMissingAnyConnection();

				auto result = ofSystemSaveDialog("mesh.obj", "Mesh file name");
				if (result.bSuccess) {
					auto node = this->getInput<Gives<ofMesh>>();
					ofMesh mesh;
					node->get(mesh);

					auto file = ofstream(result.filePath, ios::out);

					file << "# Mesh exported from Rulr v" << RULR_VERSION_STRING << endl;
					file << "# Source node \"" << node->getName() << "\" of type [" << node->getTypeName() << "]" << endl;

					file << endl;

					//vertices
					if (mesh.hasVertices()) {
						file << "# " << mesh.getNumVertices() << " vertices" << endl;
						for (const auto & vertex : mesh.getVertices()) {
							file << "v " << vertex.x << " " << vertex.y << " " << vertex.z << endl;
						}
						file << endl;
					}

					//texture coordinates
					if (mesh.hasTexCoords()) {
						file << "# " << mesh.getNumTexCoords() << " texture coordinates" << endl;
						for (const auto & textureCoordinate : mesh.getTexCoords()) {
							file << "vt " << textureCoordinate.x << " " << textureCoordinate.y << " 0" << endl;
						}
						file << endl;
					}

					//faces
					if (mesh.hasIndices() && mesh.getMode() == ofPrimitiveMode::OF_PRIMITIVE_TRIANGLES) {
						file << "# " << mesh.getNumIndices() / 3 << " faces" << endl;
						auto indexPtr = mesh.getIndexPointer();

						bool hasTextureCoordinates = mesh.hasTexCoords();
						for (int i = 0; i < mesh.getNumIndices(); i += 3) {

							file << "f";
							for (int j = 0; j < 3; j++) {
								file << " v" << indexPtr[j];
								if (hasTextureCoordinates) {
									file << "/vt" << indexPtr[j];
								}
							}
							file << endl;

							indexPtr += 3;
						}
					}

					file.close();
				}
			}

			//----------
			void Mesh::exportPLY() {
				this->throwIfMissingAnyConnection();

				auto result = ofSystemSaveDialog("mesh.ply", "Mesh file name");
				if (result.bSuccess) {
					auto node = this->getInput<Gives<ofMesh>>();
					ofMesh mesh;
					node->get(mesh);
					mesh.save(result.filePath);
				}
			}
		}
	}
}
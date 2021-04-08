#include "pch_RulrNodes.h"
#include "Mesh.h"

using namespace ofxCvGui;

namespace ofxRulr {
	namespace Nodes {
		namespace Data {
			//----------
			Mesh::Mesh() {
				RULR_NODE_INIT_LISTENER;
			}

			//----------
			string Mesh::getTypeName() const {
				return "Data::Mesh";
			}

			//----------
			void Mesh::init() {
				RULR_NODE_SERIALIZATION_LISTENERS;
				RULR_NODE_INSPECTOR_LISTENER;
			}

			//----------
			void Mesh::serialize(nlohmann::json & json) {
				this->save(this->getDefaultFilename() + ".ply");
			}

			//----------
			void Mesh::deserialize(const nlohmann::json & json) {
				auto filename = this->getDefaultFilename() + ".ply";
				if (ofFile(this->getDefaultFilename() + ".ply").exists()) {
					this->load(filename);
				}
			}

			//----------
			void Mesh::populateInspector(ofxCvGui::InspectArguments & inspectArgs) {
				auto inspector = inspectArgs.inspector;

				inspector->addTitle("Mesh details", Widgets::Title::Level::H2);
				{
					inspector->addLiveValue<size_t>("Vertices", [this]() {
						return this->mesh.getNumVertices();
					});


					inspector->addLiveValue<size_t>("Colors", [this]() {
						return this->mesh.getNumColors();
					});

					inspector->addLiveValue<size_t>("Normals", [this]() {
						return this->mesh.getNumNormals();
					});

					inspector->addLiveValue<size_t>("Texture coordinates", [this]() {
						return this->mesh.getNumTexCoords();
					});

					inspector->addLiveValue<size_t>("Indices", [this]() {
						return this->mesh.getNumIndices();
					});
				}

				inspector->addButton("Save PLY...", [this]() {
					this->save();
				});
				inspector->addButton("Load PLY...", [this]() {
					this->load();
				});
			}

			//----------
			ofMesh & Mesh::getMesh() {
				return this->mesh;
			}

			//----------
			const ofMesh & Mesh::getMesh() const {
				return this->mesh;
			}

			//----------
			void Mesh::save(string filePath) const {
				if (filePath == "") {
					auto result = ofSystemSaveDialog("mesh.ply", "Save Mesh as PLY");
					if (result.bSuccess) {
						filePath = result.filePath;
					}
					else {
						return;
					}
				}
				this->mesh.save(filePath);
			}

			//----------
			void Mesh::load(string filePath) {
				if (filePath == "") {
					auto result = ofSystemLoadDialog("Load Mesh from OBJ");
					if (result.bSuccess) {
						filePath = result.filePath;
					}
					else {
						return;
					}
				}
				this->mesh.load(filePath);
			}
		}
	}
}
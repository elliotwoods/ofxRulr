#include "pch_RulrNodes.h"
#include "Triangulate.h"

#include "../Item/Camera.h"
#include "../Item/Projector.h"
#include "./Scan/Graycode.h"

#include "ofxTriangulate.h"
#include "ofxCvGui.h"

using namespace ofxRulr::Nodes;
using namespace ofxCvGui;

namespace ofxRulr {
	namespace Nodes {
		namespace Procedure {
			//----------
			Triangulate::Triangulate() {
				RULR_NODE_INIT_LISTENER;
			}

			//----------
			void Triangulate::init() {
				RULR_NODE_INSPECTOR_LISTENER;
				RULR_NODE_SERIALIZATION_LISTENERS;

				auto cameraPin = MAKE(Graph::Pin<Item::Camera>);
				auto projectorPin = MAKE(Graph::Pin<Item::Projector>);
				auto graycodePin = MAKE(Graph::Pin<Scan::Graycode>);

				this->addInput(cameraPin);
				this->addInput(projectorPin);
				this->addInput(graycodePin);

				this->maxLength.set("Maximum length disparity [m]", 0.05f, 0.0f, 10.0f);
				this->giveColor.set("Give color", true);
				this->giveTexCoords.set("Give texture coordinates", true);
				this->drawPointSize.set("Point size for draw", 1.0f, 1.0f, 10.0f);
			}

			//----------
			string Triangulate::getTypeName() const {
				return "Procedure::Triangulate";
			}

			//----------
			void Triangulate::serialize(Json::Value & json) {

			}

			//----------
			void Triangulate::deserialize(const Json::Value & json) {

			}

			//----------
			void Triangulate::triangulate() {
				this->throwIfMissingAnyConnection();

				auto camera = this->getInput<Item::Camera>();
				auto projector = this->getInput<Item::Projector>();
				auto graycode = this->getInput<Scan::Graycode>();

				const auto & dataSet = graycode->getDataSet();

				ofxCvGui::Utils::drawProcessingNotice("Triangulating..");
				ofxTriangulate::Triangulate(dataSet, camera->getViewInWorldSpace(), projector->getViewInWorldSpace(), this->mesh, this->maxLength, this->giveColor, this->giveTexCoords);
			}

			//----------
			void Triangulate::populateInspector(ofxCvGui::InspectArguments & inspectArguments) {
				auto inspector = inspectArguments.inspector;
				
				auto triangulateButton = new Widgets::Button("Triangulate", [this]() {
					try {
						this->triangulate();
					}
					catch (std::exception e) {
						ofSystemAlertDialog(e.what());
					}
				}, OF_KEY_RETURN);
				triangulateButton->setHeight(100.0f);
				inspector->add(triangulateButton);

				inspector->add(new Widgets::Slider(this->maxLength));
				inspector->add(new Widgets::Toggle(this->giveColor));
				inspector->add(new Widgets::Toggle(this->giveTexCoords));
				inspector->add(new Widgets::Slider(this->drawPointSize));
				inspector->add(new Widgets::Button("Save ofMesh...", [this]() {
					auto result = ofSystemSaveDialog("mesh.ply", "Save mesh as PLY");
					if (result.bSuccess) {
						this->mesh.save(result.filePath);
					}
				}));
				inspector->add(new Widgets::Button("Save binary mesh...", [this]() {
					auto result = ofSystemSaveDialog("mesh.bin", "Save mesh as PLY");
					if (result.bSuccess) {
						ofstream save(ofToDataPath(result.filePath).c_str(), ios::binary);
						if (!save.is_open()) {
							ofLogError("ofxRulr::Triangulate") << "save failed to open file " << result.fileName;
							return;
						}
						auto numVertices = (uint32_t) this->mesh.getNumVertices();
						save.write((char *)& numVertices, sizeof(numVertices));
						save.write((char *) this->mesh.getVerticesPointer(), sizeof(ofVec3f)* numVertices);

						auto numTexCoords = (uint32_t) this->mesh.getNumTexCoords();
						save.write((char *)& numTexCoords, sizeof(numTexCoords));
						save.write((char *) this->mesh.getTexCoordsPointer(), sizeof(ofVec2f)* numTexCoords);

						auto numColors = (uint32_t) this->mesh.getNumColors();
						save.write((char *)& numColors, sizeof(numColors));
						save.write((char *) this->mesh.getColorsPointer(), sizeof(ofFloatColor)* numColors);

						save.close();
					}
				}));
			}

			//----------
			void Triangulate::drawWorld() {
				glPushAttrib(GL_POINT_BIT);
				glPointSize(this->drawPointSize);
				this->mesh.drawVertices();
				glPopAttrib();

				auto graycode = this->getInput<Scan::Graycode>();

				auto camera = this->getInput<Item::Camera>();
				if (camera) {
					if (graycode) {
						camera->getViewInWorldSpace().drawOnNearPlane(graycode->getDecoder().getProjectorInCamera());
					}
				}
				auto projector = this->getInput<Item::Projector>();
				if (projector) {
					if (graycode) {
						projector->getViewInWorldSpace().drawOnNearPlane(graycode->getDecoder().getCameraInProjector());
					}
				}
			}
		}
	}
}
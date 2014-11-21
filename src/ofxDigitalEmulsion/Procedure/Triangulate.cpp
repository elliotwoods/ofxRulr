#include "Triangulate.h"

#include "../Item/Camera.h"
#include "../Item/Projector.h"
#include "./Scan/Graycode.h"

#include "../../../addons/ofxTriangulate/src/ofxTriangulate.h"
#include "ofxCvGui.h"

using namespace ofxCvGui;

namespace ofxDigitalEmulsion {
	namespace Procedure {
		//----------
		Triangulate::Triangulate() {
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
			this->drawDebugRays.set("Draw debug rays", false);
			
			cameraPin->onNewConnection += [this] (shared_ptr<Item::Camera> & cameraNode) {
				cameraNode->getView()->onMouse.removeListeners(this);
				cameraNode->getView()->onMouse.addListener([this, cameraNode] (MouseArguments & mouseArgs) {
					if (mouseArgs.isLocal() && (mouseArgs.action == MouseArguments::Action::Pressed || mouseArgs.action == MouseArguments::Action::Dragged)) {
						this->cameraRay = cameraNode->getRayCamera().castPixel(mouseArgs.localNormalised * ofVec2f(cameraNode->getWidth(), cameraNode->getHeight()));
						this->intersectRay = this->cameraRay.intersect(this->projectorRay);
						this->intersectRay.color = ofColor(255);
					}
				}, this);
			};
			
			projectorPin->onNewConnection += [this] (shared_ptr<Item::Projector> & projectorNode) {
				projectorNode->getView()->onMouse.removeListeners(this);
				projectorNode->getView()->onMouse.addListener([this, projectorNode] (MouseArguments & mouseArgs) {
					if (mouseArgs.isLocal() && (mouseArgs.action == MouseArguments::Action::Pressed || mouseArgs.action == MouseArguments::Action::Dragged)) {
						this->projectorRay = projectorNode->getRayProjector().castPixel(mouseArgs.localNormalised * ofVec2f(projectorNode->getWidth(), projectorNode->getHeight()));
						this->intersectRay = this->cameraRay.intersect(this->projectorRay);
						this->intersectRay.color = ofColor(255);
					}
				}, this);
			};
		}

		//----------
		void Triangulate::init() {
		}

		//----------
		string Triangulate::getTypeName() const {
			return "Triangulate";
		}

		//----------
		ofxCvGui::PanelPtr Triangulate::getView() {
			auto view = MAKE(Panels::World);
			view->getCamera().rotate(180.0f, 0.0f, 0.0f, 1.0f);
			view->getCamera().lookAt(ofVec3f(0,0,1), ofVec3f(0,-1,0));
			view->onDrawWorld += [this] (ofCamera &) {
				this->drawWorld();
			};
			return view;			
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
			ofxTriangulate::Triangulate(dataSet, camera->getRayCamera(), projector->getRayProjector(), this->mesh, this->maxLength, this->giveColor, this->giveTexCoords);
		}

		//----------
		void Triangulate::populateInspector2(ofxCvGui::ElementGroupPtr inspector) {
			auto triangulateButton = Widgets::Button::make("Triangulate", [this] () {
				try {
					this->triangulate();
				}
				catch (std::exception e) {
					ofSystemAlertDialog(e.what());
				}
			}, OF_KEY_RETURN);
			triangulateButton->setHeight(100.0f);
			inspector->add(triangulateButton);
			inspector->add(Widgets::Slider::make(this->maxLength));
			inspector->add(Widgets::Toggle::make(this->giveColor));
			inspector->add(Widgets::Toggle::make(this->giveTexCoords));
			inspector->add(Widgets::Slider::make(this->drawPointSize));
			inspector->add(Widgets::Button::make("Save ofMesh...", [this] () {
				auto result = ofSystemSaveDialog("mesh.ply", "Save mesh as PLY");
				if (result.bSuccess) {
					this->mesh.save(result.filePath);
				}
			}));
			inspector->add(Widgets::Button::make("Save binary mesh...", [this] () {
				auto result = ofSystemSaveDialog("mesh.bin", "Save mesh as PLY");
				if (result.bSuccess) {
					ofstream save(ofToDataPath(result.filePath).c_str(), ios::binary);
					if (!save.is_open()) {
						ofLogError("ofxDigitalEmulsion::Triangulate") << "save failed to open file " << result.fileName;
						return;
					}
					auto numVertices = (uint32_t) this->mesh.getNumVertices();
					save.write((char *) & numVertices, sizeof(numVertices));
					save.write((char *) this->mesh.getVerticesPointer(), sizeof(ofVec3f) * numVertices);

					auto numTexCoords = (uint32_t) this->mesh.getNumTexCoords();
					save.write((char *) & numTexCoords, sizeof(numTexCoords));
					save.write((char *) this->mesh.getTexCoordsPointer(), sizeof(ofVec2f) * numTexCoords);
					
					auto numColors = (uint32_t) this->mesh.getNumColors();
					save.write((char *) & numColors, sizeof(numColors));
					save.write((char *) this->mesh.getColorsPointer(), sizeof(ofColor) * numColors);
					
					save.close();
				}
			}));
			
			inspector->add(Widgets::Spacer::make());
			inspector->add(Widgets::Toggle::make(this->drawDebugRays));
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
				camera->drawWorld();
				if (graycode) {
					camera->getRayCamera().drawOnNearPlane(graycode->getDecoder().getProjectorInCamera());
				}
			}
			auto projector = this->getInput<Item::Projector>();
			if (projector) {
				projector->drawWorld();
				if (graycode) {
					projector->getRayProjector().drawOnNearPlane(graycode->getDecoder().getCameraInProjector());
				}
			}
			
			if (this->drawDebugRays) {
				this->cameraRay.draw();
				this->projectorRay.draw();
				this->intersectRay.draw();
			}
		}
	}
}
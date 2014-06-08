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
			this->inputPins.push_back(MAKE(Graph::Pin<Item::Camera>));
			this->inputPins.push_back(MAKE(Graph::Pin<Item::Projector>));
			this->inputPins.push_back(MAKE(Graph::Pin<Scan::Graycode>));

			this->maxLength.set("Maximum length disparity [m]", 0.05f, 0.0f, 10.0f);
			this->giveColor.set("Give color", true);
			this->giveTexCoords.set("Give texture coordinates", true);
			this->drawPointSize.set("Point size for draw", 1.0f, 1.0f, 10.0f);
		}

		//----------
		string Triangulate::getTypeName() const {
			return "Triangulate";
		}

		//----------
		Graph::PinSet Triangulate::getInputPins() const {
			return this->inputPins;
		}

		//----------
		ofxCvGui::PanelPtr Triangulate::getView() {
			auto view = MAKE(Panels::World);
			view->getCamera().rotate(180.0f, 0.0f, 0.0f, 1.0f);
			view->getCamera().lookAt(ofVec3f(0,0,1), ofVec3f(0,-1,0));
			view->onDrawWorld += [this] (ofCamera &) {
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
			this->throwIfMissingAConnection();
			
			auto camera = this->getInput<Item::Camera>();
			auto projector = this->getInput<Item::Projector>();
			auto graycode = this->getInput<Scan::Graycode>();

			const auto & dataSet = graycode->getDataSet();

			ofxCvGui::Utils::drawProcessingNotice("Triangulating..");
			ofxTriangulate::Triangulate(dataSet, camera->getRayCamera(), projector->getRayProjector(), this->mesh, this->maxLength, this->giveColor, this->giveTexCoords);
		}

		//----------
		void Triangulate::populateInspector2(ofxCvGui::ElementGroupPtr inspector) {
			inspector->add(Widgets::Button::make("Triangulate", [this] () {
				try {
					this->triangulate();
				} catch (std::exception e) {
					ofSystemAlertDialog(e.what());
				}
			}));
			inspector->add(Widgets::Slider::make(this->maxLength));
			inspector->add(Widgets::Toggle::make(this->giveColor));
			inspector->add(Widgets::Toggle::make(this->giveTexCoords));
			inspector->add(Widgets::Slider::make(this->drawPointSize));
		}
	}
}
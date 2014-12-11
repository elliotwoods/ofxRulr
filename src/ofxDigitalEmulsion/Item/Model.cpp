#include "Model.h"

#include <ofxCvGui/Widgets/Button.h>
#include <ofxCvGui/Widgets/LiveValue.h>
#include <ofxCvGui/Widgets/Toggle.h>
#include <ofxCvGui/Widgets/Title.h>
#include <ofxCvGui/Widgets/Slider.h>
#include <ofxCvGui/Panels/World.h>

#include "ofSystemUtils.h"

using namespace ofxCvGui;
using namespace ofxCvGui::Widgets;

namespace ofxDigitalEmulsion {
	namespace Item {
		//----------
		Model::Model() {

		}

		//----------
		string Model::getTypeName() const {
			return "Item::Model";
		}

		//----------
		void Model::init() {
			this->drawVertices.set("Draw vertices", false);
			this->drawWireframe.set("Draw wireframe", false);
			this->drawFaces.set("Draw faces", true);

			this->flipX.set("Flip X", false);
			this->flipY.set("Flip Y", true);
			this->flipZ.set("Flip Z", true);
			this->inputUnitScale.set("Input unit scale", 0.0254f, 1e-6f, 1e6f);

			auto view = make_shared<Panels::World>();
			view->onDrawWorld += [this](ofCamera &) {
				this->drawWorld();
			};
			this->view = view;
		}

		//----------
		ofxCvGui::PanelPtr Model::getView() {
			return this->view;
		}

		//----------
		void Model::drawWorld() {
			ofPushMatrix();
			if (this->flipX) {
				ofScale(-1, 1, 1);
			}
			if (this->flipY) {
				ofScale(1, -1, 1);
			}
			if (this->flipZ) {
				ofScale(1, 1, -1);
			}
			const auto scale = this->inputUnitScale.get();
			ofScale(scale, scale, scale);

			if (this->drawVertices) {
				glPushAttrib(GL_POINT_BIT);
				glEnable(GL_POINT_SMOOTH);
				glPointSize(3.0f);
				this->modelLoader.drawVertices();
				glPopAttrib();
			}
			if (this->drawWireframe) {
				this->modelLoader.drawWireframe();
			}
			if (this->drawFaces) {
				this->modelLoader.drawFaces();
			}
			ofPopMatrix();
		}

		//----------
		void Model::serialize(Json::Value & json) {
			Utils::Serializable::serialize(this->drawVertices, json);
			Utils::Serializable::serialize(this->drawWireframe, json);
			Utils::Serializable::serialize(this->drawFaces, json);
			Utils::Serializable::serialize(this->flipX, json);
			Utils::Serializable::serialize(this->flipY, json);
			Utils::Serializable::serialize(this->flipZ, json);
			Utils::Serializable::serialize(this->inputUnitScale, json);
		}

		//----------
		void Model::deserialize(const Json::Value & json) {
			Utils::Serializable::deserialize(this->drawVertices, json);
			Utils::Serializable::deserialize(this->drawWireframe, json);
			Utils::Serializable::deserialize(this->drawFaces, json);
			Utils::Serializable::deserialize(this->flipX, json);
			Utils::Serializable::deserialize(this->flipY, json);
			Utils::Serializable::deserialize(this->flipZ, json);
			Utils::Serializable::deserialize(this->inputUnitScale, json);
		}

		//----------
		void Model::populateInspector2(ofxCvGui::ElementGroupPtr inspector) {
			auto loadButton = make_shared<ofxCvGui::Widgets::Button>("Load model...", [this]() {
				auto result = ofSystemLoadDialog("");
				this->modelLoader.loadModel(result.filePath);
			});
			inspector->add(loadButton);

			inspector->add(make_shared<Title>("Draw options", Title::Level::H3));
			inspector->add(make_shared<Toggle>(this->drawVertices));
			inspector->add(make_shared<Toggle>(this->drawWireframe));
			inspector->add(make_shared<Toggle>(this->drawFaces));

			inspector->add(make_shared<Title>("Model transform", Title::Level::H3));
			inspector->add(make_shared<Toggle>(this->flipX));
			inspector->add(make_shared<Toggle>(this->flipY));
			inspector->add(make_shared<Toggle>(this->flipZ));
			inspector->add(make_shared<Slider>(this->inputUnitScale));
		}
	}
}
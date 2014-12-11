#include "Model.h"

#include <ofxCvGui/Widgets/Button.h>
#include <ofxCvGui/Widgets/LiveValue.h>
#include <ofxCvGui/Widgets/Toggle.h>

#include "ofSystemUtils.h"

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

		}

		//----------
		ofxCvGui::PanelPtr Model::getView() {
			auto panel = make_shared<ofxCvGui::Panels::Base>();
			return panel;
		}

		//----------
		void Model::drawWorld() {
			this->modelLoader.drawFaces();
		}

		//----------
		void Model::serialize(Json::Value & json) {
			
		}

		//----------
		void Model::deserialize(const Json::Value & json) {

		}

		//----------
		void Model::populateInspector2(ofxCvGui::ElementGroupPtr inspector) {
			auto loadButton = make_shared<ofxCvGui::Widgets::Button>("Load model...", [this]() {
				auto result = ofSystemLoadDialog("");
				this->modelLoader.loadModel(result.filePath);
			});
			inspector->add(loadButton);
		}
	}
}
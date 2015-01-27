#include "Summary.h"

#include "ofxCvGui/Widgets/Toggle.h"
#include "ofxCvGui/Widgets/Slider.h"

namespace ofxDigitalEmulsion {
	namespace Graph {
		//----------
		Summary::Summary(const Utils::Set<Graph::Node> & world) :
		world(world) {
			this->view = MAKE(ofxCvGui::Panels::World);
			this->view->onDrawWorld += [this](ofCamera &) {
				if (this->showGrid) {
					ofDrawGrid(this->gridScale);
				}
				for (const auto node : this->world) {
					node->drawWorld();
				}
			};

			this->showCursor.addListener(this, & Summary::callbackShowCursor);
		}

		//----------
		void Summary::init() {
			this->showCursor.set("Show Cursor", false);
			this->showGrid.set("Show Grid", true);
			this->gridScale.set("Grid scale", 10.0f, 0.01f, 100.0f);
			this->view->setGridEnabled(false);

			OFXDIGITALEMULSION_NODE_STANDARD_LISTENERS
		}

		//----------
		string Summary::getTypeName() const {
			return "Summary";
		}

		//----------
		ofxCvGui::PanelPtr Summary::getView() {
			return this->view;
		}

		//----------
		void Summary::update() {

		}

		//----------
		void Summary::serialize(Json::Value & json) {
			Utils::Serializable::serialize(this->showCursor, json);
			Utils::Serializable::serialize(this->showGrid, json);
			Utils::Serializable::serialize(this->gridScale, json);

			auto & camera = this->view->getCamera();
			auto & cameraJson = json["Camera"];
			cameraJson["Transform"] << camera.getGlobalTransformMatrix();
		}

		//----------
		void Summary::deserialize(const Json::Value & json) {
			Utils::Serializable::deserialize(this->showCursor, json);
			this->view->setCursorEnabled(this->showCursor);

			Utils::Serializable::deserialize(this->showGrid, json);
			Utils::Serializable::deserialize(this->gridScale, json);

			if (json.isMember("Camera")) {
				auto & cameraJson = json["Camera"];
				auto & camera = this->view->getCamera();
				ofMatrix4x4 transform;
				cameraJson["Transform"] >> transform;
				camera.setTransformMatrix(transform);

				//nudge the camera
				camera.move(ofVec3f());
			}
		}

		//----------
		void Summary::populateInspector(ofxCvGui::ElementGroupPtr inspector) {
			inspector->add(MAKE(ofxCvGui::Widgets::Toggle, this->showCursor));
			inspector->add(MAKE(ofxCvGui::Widgets::Toggle, this->showGrid));
			inspector->add(MAKE(ofxCvGui::Widgets::Slider, this->gridScale));
		}

		//----------
		void Summary::callbackShowCursor(bool & showCursor) {
			this->view->setCursorEnabled(showCursor);
		}
	}
}
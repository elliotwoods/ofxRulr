#include "Summary.h"

#include "ofxCvGui/Widgets/Toggle.h"

namespace ofxDigitalEmulsion {
	namespace Graph {
		//----------
		Summary::Summary(const Utils::Set<Graph::Node> & world) :
		world(world) {
			this->view = MAKE(ofxCvGui::Panels::World);
			this->view->onDrawWorld += [this](ofCamera &) {
				if (this->showGrid) {
					ofDrawGrid(10.0f);
				}
				for (const auto node : this->world) {
					node->drawWorld();
				}
			};

			this->showCursor.addListener(this, & Summary::callbackShowCursor);

			this->showCursor.set("Show Cursor", false);
			this->showGrid.set("Show Grid", true);
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
			this->view->setCursorEnabled(this->showCursor);
			this->view->setGridEnabled(this->showGrid);
		}

		//----------
		void Summary::serialize(Json::Value & json) {
			Utils::Serializable::serialize(this->showCursor, json);
			Utils::Serializable::serialize(this->showGrid, json);
		}

		//----------
		void Summary::deserialize(const Json::Value & json) {
			Utils::Serializable::deserialize(this->showCursor, json);
			Utils::Serializable::deserialize(this->showGrid, json);
			this->view->setCursorEnabled(this->showCursor);
		}

		//----------
		void Summary::populateInspector2(ofxCvGui::ElementGroupPtr inspector) {
			inspector->add(MAKE(ofxCvGui::Widgets::Toggle, this->showCursor));
			inspector->add(MAKE(ofxCvGui::Widgets::Toggle, this->showGrid));
		}

		//----------
		void Summary::callbackShowCursor(bool & showCursor) {
			this->view->setCursorEnabled(showCursor);
		}
	}
}
#include "Summary.h"

#include "ofxDigitalEmulsion/Item/Base.h";
#include "ofxCvGui/Widgets/Toggle.h"

namespace ofxDigitalEmulsion {
	namespace Graph {
		//----------
		Summary::Summary(const Utils::Set<Graph::Node> & world) :
		world(world) {
			this->view = MAKE(ofxCvGui::Panels::World);
			this->view->onDrawWorld += [this](ofCamera &) {
				for (const auto node : this->world) {
					auto item = dynamic_pointer_cast<Item::Base>(node);
					if (item) {
						item->drawWorld();
					}
				}
			};
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
		}

		//----------
		void Summary::populateInspector2(ofxCvGui::ElementGroupPtr inspector) {
			inspector->add(MAKE(ofxCvGui::Widgets::Toggle, this->showCursor));
			inspector->add(MAKE(ofxCvGui::Widgets::Toggle, this->showGrid));
		}
	}
}
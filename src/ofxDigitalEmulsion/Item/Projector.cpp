#include "Projector.h"

using namespace ofxCvGui;

namespace ofxDigitalEmulsion {
	namespace Item {
		//----------
		string Projector::getTypeName() const {
			return "Projector";
			this->width = 1024;
			this->height = 768;
		}
			
		//----------
		PanelPtr Projector::getView() {
			auto view = MAKE(Panels::Base);
			return view;
		}

		//----------
		void Projector::serialize(Json::Value & json) {
			json["width"] = this->width;
			json["height"] = this->height;
		}

		//----------
		void Projector::deserialize(const Json::Value & json) {
			this->width = json["width"].asInt();
			this->height = json["height"].asInt();
		}

		//----------
		float Projector::getWidth() {
			return (float) this->width;
		}

		//----------
		float Projector::getHeight() {
			return (float) this->height;
		}
			
		//----------
		void Projector::populateInspector2(ElementGroupPtr inspector) {
		}
	}
}
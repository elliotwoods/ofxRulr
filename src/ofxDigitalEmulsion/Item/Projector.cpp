#include "Projector.h"

using namespace ofxCvGui;

namespace ofxDigitalEmulsion {
	namespace Item {
		//----------
		string Projector::getTypeName() const {
			return "Projector";
		}
			
		//----------
		PanelPtr Projector::getView() {
			auto view = MAKE(Panels::Base);
			return view;
		}

		//----------
		void Projector::serialize(Json::Value & json) {
		}

		//----------
		void Projector::deserialize(Json::Value & json) {
		}

		//----------
		float Projector::getWidth() {
			return 1024;
		}

		//----------
		float Projector::getHeight() {
			return 768;
		}
			
		//----------
		void Projector::populateInspector2(ElementGroupPtr inspector) {
		}
	}
}
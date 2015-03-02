#include "Projector.h"

#include "ofxCvGui.h"

using namespace ofxCvGui;

namespace ofxDigitalEmulsion {
	namespace Item {
		//----------
		Projector::Projector() {
			OFXDIGITALEMULSION_NODE_INIT_LISTENER;
		}

		//----------
		void Projector::init() {
			OFXDIGITALEMULSION_NODE_SERIALIZATION_LISTENERS;
			OFXDIGITALEMULSION_NODE_INSPECTOR_LISTENER;

			this->projector.setDefaultNear(0.05f);
			this->projector.setDefaultFar(2.0f);

			this->resolutionWidth.set("Resolution width", 1024.0f, 1.0f, 8 * 1280.0f);
			this->resolutionHeight.set("Resolution height", 768.0f, 1.0f, 8 * 800.0f);
			this->resolutionWidth.addListener(this, &Projector::projectorParameterCallback);
			this->resolutionHeight.addListener(this, &Projector::projectorParameterCallback);
		}

		//----------
		string Projector::getTypeName() const {
			return "Item::Projector";
		}
			
		//----------
		void Projector::serialize(Json::Value & json) {
			Utils::Serializable::serialize(this->resolutionWidth, json);
			Utils::Serializable::serialize(this->resolutionHeight, json);
		}

		//----------
		void Projector::deserialize(const Json::Value & json) {
			Utils::Serializable::deserialize(this->resolutionWidth, json);
			Utils::Serializable::deserialize(this->resolutionHeight, json);
		}

		//----------
		void Projector::setWidth(float width) {
			this->resolutionWidth = width;
		}

		//----------
		void Projector::setHeight(float height) {
			this->resolutionHeight = height;
		}

		//----------
		float Projector::getWidth() const {
			return this->resolutionWidth;
		}

		//----------
		float Projector::getHeight() const {
			return this->resolutionHeight;
		}

		//----------
		void Projector::projectorParameterCallback(float &) {
			this->rebuildViewFromParameters();
		}
		//----------
		void Projector::populateInspector(ElementGroupPtr inspector) {
			inspector->add(Widgets::EditableValue<float>::make(this->resolutionWidth));
			inspector->add(Widgets::EditableValue<float>::make(this->resolutionHeight));
		}
	}
}
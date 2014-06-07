#include "Projector.h"

using namespace ofxCvGui;

namespace ofxDigitalEmulsion {
	namespace Item {
		//----------
		Projector::Projector() {
			this->projector.setWidth(1024);
			this->projector.setHeight(768);
			this->projector.setDefaultNear(0.05f);
			this->projector.setDefaultFar(2.0f);
		}

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
			json["width"] = this->projector.getWidth();
			json["height"] = this->projector.getHeight();
		}

		//----------
		void Projector::deserialize(const Json::Value & json) {
			if(json["width"].asBool() && json["height"].asBool()) {
				this->projector.setWidth(json["width"].asInt());
				this->projector.setHeight(json["height"].asInt());
			}
		}

		//----------
		float Projector::getWidth() {
			return (float) this->projector.getWidth();
		}

		//----------
		float Projector::getHeight() {
			return (float) this->projector.getHeight();
		}

		//----------
		void Projector::setIntrinsics(cv::Mat cameraMatrix) {
			const auto newProjection = ofxCv::makeProjectionMatrix(cameraMatrix, cv::Size(projector.getWidth(), projector.getHeight()));
			cout << "old projection " << projector.getProjectionMatrix() << endl;
			cout << "new projection " << newProjection << endl;
			projector.setProjection(newProjection);
		}
		
		//----------
		void Projector::setExtrinsics(cv::Mat rotation, cv::Mat translation) {
			this->projector.setView(ofxCv::makeMatrix(rotation, translation));
		}

		//----------
		void Projector::drawWorld() {
			this->projector.draw();
		}
			
		//----------
		void Projector::populateInspector2(ElementGroupPtr inspector) {

		}
	}
}
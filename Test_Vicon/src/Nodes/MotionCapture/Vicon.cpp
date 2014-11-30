#include "Vicon.h"

namespace ofxDigitalEmulsion {
	namespace MotionCapture {
		//---------
		Vicon::Vicon() {

		}

		//---------
		string Vicon::getTypeName() const {
			return "Tracker::Vicon";
		}

		//---------
		void Vicon::init() {
			//make a blank gui item
			this->view = make_shared<ofxCvGui::Panels::Base>();
		}

		//---------
		void Vicon::update() {

		}

		//---------
		void Vicon::serialize(Json::Value & json) {

		}

		//---------
		void Vicon::deserialize(const Json::Value & json) {

		}

		//---------
		ofxCvGui::PanelPtr Vicon::getView() {
			return this->view;
		}

		//---------
		void Vicon::populateInspector2(ofxCvGui::ElementGroupPtr inspector) {

		}

		//---------
		shared_ptr<Frame> Vicon::getCurrentFrame() {
			return this->currentFrame;
		}
	}
}
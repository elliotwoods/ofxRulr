#include "KinectV2.h"
#include "ofxCvGui.h"

using namespace ofxCvGui;

namespace ofxDigitalEmulsion {
	namespace External {
		namespace Item {
			//----------
			KinectV2::KinectV2() {
				auto view = MAKE(ofxCvGui::Panels::World);
				view->onDrawWorld += [this](ofCamera &) {
					this->drawWorld();
				};
				this->view = view;

				this->device = MAKE(ofxKinectForWindows2::Device);
				this->device->open();
				this->device->initDepthSource();
				this->device->initColorSource();
				this->device->initBodySource();
			}

			//----------
			string KinectV2::getTypeName() const {
				return "KinectV2";
			}

			//----------
			void KinectV2::update() {
				if (this->device) {
					this->device->update();
				}
			}

			//----------
			ofxCvGui::PanelPtr KinectV2::getView() {
				return this->view;
			}

			//----------
			void KinectV2::serialize(Json::Value &) {

			}

			//----------
			void KinectV2::deserialize(const Json::Value &) {

			}

			//----------
			void KinectV2::drawWorld() {
				if (this->device) {
					this->device->drawPrettyMesh();
				}
			}

			//----------
			void KinectV2::populateInspector2(ofxCvGui::ElementGroupPtr inspector) {

			}
		}
	}
}
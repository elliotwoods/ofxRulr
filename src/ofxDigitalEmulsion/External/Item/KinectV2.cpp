#include "KinectV2.h"

#include "../../External/Manager.h"

#include "ofxCvGui/Panels/Draws.h"
#include "ofxCvGui/Widgets/MultipleChoice.h"

using namespace ofxCvGui;

namespace ofxDigitalEmulsion {
	namespace Item {
		//----------
		KinectV2::KinectV2() {
			
		}

		//----------
		void KinectV2::init() {
			auto view = MAKE(ofxCvGui::Panels::Groups::Grid);
			this->view = view;

			this->device = MAKE(ofxKinectForWindows2::Device);
			this->device->open();
			this->device->initDepthSource();
			this->device->initColorSource();
			this->device->initBodySource();

			this->view->add(make_shared<ofxCvGui::Panels::Draws>(this->device->getColorSource()->getTextureReference()));
			this->view->add(make_shared<ofxCvGui::Panels::Draws>(this->device->getDepthSource()->getTextureReference()));

			this->viewType.set("Play state", 0, 0, 1);
			this->viewType.set("View type", 3, 0, 3);

			OFXDIGITALEMULSION_NODE_STANDARD_LISTENERS
		}

		//----------
		string KinectV2::getTypeName() const {
			return "Item::KinectV2";
		}

		//----------
		void KinectV2::update() {
			if (this->device && this->playState == 0) {
				this->device->update();
			}
		}

		//----------
		ofxCvGui::PanelPtr KinectV2::getView() {
			return this->view;
		}

		//----------
		void KinectV2::serialize(Json::Value & json) {
			ofxDigitalEmulsion::Utils::Serializable::serialize(this->viewType, json);
		}

		//----------
		void KinectV2::deserialize(const Json::Value & json) {
			ofxDigitalEmulsion::Utils::Serializable::deserialize(this->viewType, json);
		}

		//----------
		void KinectV2::drawObject() {
			if (this->device) {
				switch (this->viewType.get()) {
				case 1:
					this->device->getBodySource()->drawBodies();
					break;
				case 2:
					this->device->drawWorld();
					break;
				default:
					break;
				}
			}
		}

		//----------
		shared_ptr<ofxKinectForWindows2::Device> KinectV2::getDevice() {
			return this->device;
		}

		//----------
		void KinectV2::populateInspector(ofxCvGui::ElementGroupPtr inspector) {
			auto selectPlayState = make_shared<ofxCvGui::Widgets::MultipleChoice>("Play state");
			selectPlayState->addOption("Play");
			selectPlayState->addOption("Pause");
			selectPlayState->entangle(this->playState);
			inspector->add(selectPlayState);

			auto selectViewType = make_shared<ofxCvGui::Widgets::MultipleChoice>("3D View");
			selectViewType->addOption("None");
			selectViewType->addOption("Bodies");
			selectViewType->addOption("All");
			selectViewType->entangle(this->viewType);
			inspector->add(selectViewType);
		}
	}
}
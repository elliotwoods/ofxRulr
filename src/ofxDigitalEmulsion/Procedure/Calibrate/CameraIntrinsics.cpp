#include "CameraIntrinsics.h"

#include "../../Item/Checkerboard.h"
#include "../../Item/Camera.h"

using namespace ofxDigitalEmulsion::Graph;

namespace ofxDigitalEmulsion {
	namespace Procedure {
		namespace Calibrate {
			//----------
			CameraIntrinsics::CameraIntrinsics() {
				this->inputPins.push_back(MAKE(Pin<Item::Checkerboard>));
				this->inputPins.push_back(MAKE(Pin<Item::Camera>));
			}

			//----------
			string CameraIntrinsics::getTypeName() const {
				return "CameraIntrinsics";
			}

			//----------
			PinSet CameraIntrinsics::getInputPins() {
				return this->inputPins;
			}

			//----------
			ofxCvGui::PanelPtr CameraIntrinsics::getView() {
				auto view = MAKE(ofxCvGui::Panels::Base);
				return view;
			}
		}
	}
}
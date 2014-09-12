#include "ProjectorFromKinectV2.h"

#include "../../../Item/Projector.h"
#include "../../../Device/ProjectorOutput.h"
#include "../../Item/KinectV2.h"

#include "../../../Utils/Utils.h"

#include "ofxCvGui.h"
#include "ofxCvMin.h"

using namespace ofxDigitalEmulsion::Graph;
using namespace ofxCvGui;

using namespace ofxCv;
using namespace cv;

namespace ofxDigitalEmulsion {
	namespace Procedure {
		namespace Calibrate {
			//----------
			ProjectorFromKinectV2::ProjectorFromKinectV2() {
				this->inputPins.push_back(MAKE(Pin<Item::Projector>));
				this->inputPins.push_back(MAKE(Pin<Item::Projector>));

				this->checkerboardScale.set("Checkerboard Scale", 0.2f, 0.01f, 1.0f);
				this->checkerboardCornersX.set("Checkerboard Corners X", 5, 1, 10);
				this->checkerboardCornersY.set("Checkerboard Corners Y", 5, 1, 10);
				this->checkerboardPositionX.set("Checkerboard Position X", 0, -1, 1);
				this->checkerboardPositionY.set("Checkerboard Position Y", 0, -1, 1);

				auto view = MAKE(ofxCvGui::Panels::World);
				view->onDrawWorld += [this](ofCamera &) {
					this->drawWorld();
				};
				this->view = view;
			}

			//----------
			string ProjectorFromKinectV2::getTypeName() const {
				return "ProjectorFromKinectV2";
			}

			//----------
			Graph::PinSet ProjectorFromKinectV2::getInputPins() const {
				return this->inputPins;
			}

			//----------
			ofxCvGui::PanelPtr ProjectorFromKinectV2::getView() {
				return view;
			}

			//----------
			void ProjectorFromKinectV2::update() {
				auto projectorOutput = this->getInput<Device::ProjectorOutput>();
				if (projectorOutput) {
					if (projectorOutput->isWindowOpen()) {
						projectorOutput->getFbo().begin();
						ofSetMatrixMode(ofMatrixMode::OF_MATRIX_PROJECTION);
						ofLoadIdentityMatrix();
						ofSetMatrixMode(ofMatrixMode::OF_MATRIX_MODELVIEW);
						ofLoadIdentityMatrix();

						auto checkboardMesh = ofxCv::makeCheckerboardMesh(cv::Size(this->checkerboardCornersX, this->checkerboardCornersY), this->checkerboardScale);
						ofTranslate(this->checkerboardPositionX, this->checkerboardPositionY);
						checkboardMesh.draw();

						projectorOutput->getFbo().end();
					}
				}
			}

			//----------
			void ProjectorFromKinectV2::serialize(Json::Value & json) {
				ofxDigitalEmulsion::Utils::Serializable::serialize(this->checkerboardScale, json);
				ofxDigitalEmulsion::Utils::Serializable::serialize(this->checkerboardCornersX, json);
				ofxDigitalEmulsion::Utils::Serializable::serialize(this->checkerboardCornersY, json);
				ofxDigitalEmulsion::Utils::Serializable::serialize(this->checkerboardPositionX, json);
				ofxDigitalEmulsion::Utils::Serializable::serialize(this->checkerboardPositionY, json);
			}

			//----------
			void ProjectorFromKinectV2::deserialize(const Json::Value & json) {
				ofxDigitalEmulsion::Utils::Serializable::deserialize(this->checkerboardScale, json);
				ofxDigitalEmulsion::Utils::Serializable::deserialize(this->checkerboardCornersX, json);
				ofxDigitalEmulsion::Utils::Serializable::deserialize(this->checkerboardCornersY, json);
				ofxDigitalEmulsion::Utils::Serializable::deserialize(this->checkerboardPositionX, json);
				ofxDigitalEmulsion::Utils::Serializable::deserialize(this->checkerboardPositionY, json);
			}

			//----------
			void ProjectorFromKinectV2::addCapture() {
				this->throwIfMissingAnyConnection();

				auto kinect = this->getInput<Item::KinectV2>();
				
			}

			//----------
			void ProjectorFromKinectV2::populateInspector2(ofxCvGui::ElementGroupPtr inspector) {
				auto slider = MAKE(ofxCvGui::Widgets::Slider, this->checkerboardScale);
				slider->addIntValidator();
				inspector->add(slider);

				slider = MAKE(ofxCvGui::Widgets::Slider, this->checkerboardCornersX);
				slider->addIntValidator();
				inspector->add(slider);

				slider = MAKE(ofxCvGui::Widgets::Slider, this->checkerboardCornersY);
				slider->addIntValidator();
				inspector->add(slider);

				slider = MAKE(ofxCvGui::Widgets::Slider, this->checkerboardPositionX);
				slider->addIntValidator();
				inspector->add(slider);

				slider = MAKE(ofxCvGui::Widgets::Slider, this->checkerboardPositionY);
				slider->addIntValidator();
				inspector->add(slider);

				auto addButton = MAKE(ofxCvGui::Widgets::Button, "Add Capture", [this]() {
					try {
						this->addCapture();
					}
					OFXDIGITALEMULSION_CATCH_ALL_TO_ALERT
				}, OF_KEY_RETURN);
				addButton->setHeight(100.0f);
				inspector->add(addButton);
			}

			//----------
			void ProjectorFromKinectV2::drawWorld() {
				auto kinect = this->getInput<Item::KinectV2>();
				if (kinect) {
					kinect->getDevice()->drawPrettyMesh();
				}
			}
		}
	}
}
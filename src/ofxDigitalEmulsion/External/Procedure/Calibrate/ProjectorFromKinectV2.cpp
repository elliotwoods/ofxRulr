#include "ProjectorFromKinectV2.h"

#include "../../../Item/Projector.h"
#include "../../../Device/ProjectorOutput.h"
#include "../../Item/KinectV2.h"

#include "../../../Utils/Utils.h"

#include "ofxCvGui/Panels/Groups/Grid.h"
#include "ofxCvGui/Panels/World.h"
#include "ofxCvGui/Panels/Draws.h"

#include "ofxCvGui/Widgets/Slider.h"
#include "ofxCvGui/Widgets/Button.h"

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
				this->inputPins.push_back(MAKE(Pin<Item::KinectV2>));
				this->inputPins.push_back(MAKE(Pin<Device::ProjectorOutput>));

				this->checkerboardScale.set("Checkerboard Scale", 0.2f, 0.01f, 1.0f);
				this->checkerboardCornersX.set("Checkerboard Corners X", 5, 1, 10);
				this->checkerboardCornersY.set("Checkerboard Corners Y", 5, 1, 10);
				this->checkerboardPositionX.set("Checkerboard Position X", 0, -1, 1);
				this->checkerboardPositionY.set("Checkerboard Position Y", 0, -1, 1);
				this->checkerboardBrightness.set("Checkerboard Brightness", 0.5, 0, 1);
			}

			//----------
			void ProjectorFromKinectV2::init() {
				auto view = MAKE(ofxCvGui::Panels::Groups::Grid);
				auto worldView = MAKE(ofxCvGui::Panels::World);
				worldView->onDrawWorld += [this](ofCamera &) {
					this->drawWorld();
				};
				view->add(worldView);
				auto colorSource = this->getInput<Item::KinectV2>()->getDevice()->getColorSource();
				view->add(MAKE(ofxCvGui::Panels::Draws, colorSource->getTextureReference()));
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

						ofPushStyle();
						ofSetColor(255.0f * this->checkerboardBrightness);
						ofRect(-1, -1, 2, 2);
						ofPopStyle();

						ofTranslate(this->checkerboardPositionX, this->checkerboardPositionY);
						auto checkerboardMesh = ofxCv::makeCheckerboardMesh(cv::Size(this->checkerboardCornersX, this->checkerboardCornersY), this->checkerboardScale);
						auto & colors = checkerboardMesh.getColors();
						for (auto & color : colors) {
							color *= this->checkerboardBrightness;
						}
						checkerboardMesh.draw();

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
				ofxDigitalEmulsion::Utils::Serializable::serialize(this->checkerboardBrightness, json);

				auto & jsonCorrespondences = json["correspondences"];
				int index = 0;
				for (const auto & correspondence : this->correspondences) {
					auto & jsonCorrespondence = jsonCorrespondences[index++];
					for (int i = 0; i < 3; i++) {
						jsonCorrespondence["world"][i] = correspondence.world[i];
					}
					for (int i = 0; i < 2; i++) {
						jsonCorrespondence["projector"][i] = correspondence.projector[i];
					}
				}
			}

			//----------
			void ProjectorFromKinectV2::deserialize(const Json::Value & json) {
				ofxDigitalEmulsion::Utils::Serializable::deserialize(this->checkerboardScale, json);
				ofxDigitalEmulsion::Utils::Serializable::deserialize(this->checkerboardCornersX, json);
				ofxDigitalEmulsion::Utils::Serializable::deserialize(this->checkerboardCornersY, json);
				ofxDigitalEmulsion::Utils::Serializable::deserialize(this->checkerboardPositionX, json);
				ofxDigitalEmulsion::Utils::Serializable::deserialize(this->checkerboardPositionY, json);
				ofxDigitalEmulsion::Utils::Serializable::deserialize(this->checkerboardBrightness, json);

				this->correspondences.clear();
				auto & jsonCorrespondences = json["correspondences"];
				for (const auto & jsonCorrespondence : jsonCorrespondences) {
					Correspondence correspondence;
					for (int i = 0; i < 3; i++) {
						correspondence.world[i] = jsonCorrespondence["world"][i].asFloat();
					}
					for (int i = 0; i < 2; i++) {
						correspondence.projector[i] = jsonCorrespondence["projector"][i].asFloat();
					}
					this->correspondences.push_back(correspondence);
				}
			}

			//----------
			void ProjectorFromKinectV2::addCapture() {
				this->throwIfMissingAnyConnection();

				auto kinectNode = this->getInput<Item::KinectV2>();
				auto kinectDevice = kinectNode->getDevice();
				auto colorImage = ofxCv::toCv(kinectDevice->getColorSource()->getPixelsRef());

				vector<ofVec2f> cameraPoints;
				bool success = ofxCv::findChessboardCornersPreTest(colorImage, cv::Size(this->checkerboardCornersX, this->checkerboardCornersY), toCv(cameraPoints));
				if (success) {
					ofxDigitalEmulsion::Utils::playSuccessSound();
					auto cameraToWorldMap = kinectDevice->getDepthSource()->getColorToWorldMap();
					auto cameraToWorldPointer = (ofVec3f*) cameraToWorldMap.getPixels();
					auto cameraWidth = cameraToWorldMap.getWidth();
					auto checkerboardCorners = toOf(ofxCv::makeCheckerboardPoints(cv::Size(this->checkerboardCornersX, this->checkerboardCornersY), this->checkerboardScale, true));
					for (auto checkerboardCorner : checkerboardCorners) {
						checkerboardCorner += ofVec3f(this->checkerboardPositionX, this->checkerboardPositionY, 0.0f);
					}
					int pointIndex = 0;
					for (auto cameraPoint : cameraPoints) {
						Correspondence correspondence;

						correspondence.world = cameraToWorldPointer[(int) cameraPoint.x + (int) cameraPoint.y * cameraWidth];
						correspondence.projector = (ofVec2f)checkerboardCorners[pointIndex];
						this->correspondences.push_back(correspondence);
						pointIndex++;
					}
				}
				else {
					ofxDigitalEmulsion::Utils::playFailSound();
				}
			}

			//----------
			void ProjectorFromKinectV2::calibrate() {
				this->throwIfMissingAnyConnection();
			}

			//----------
			void ProjectorFromKinectV2::populateInspector2(ofxCvGui::ElementGroupPtr inspector) {
				auto slider = MAKE(ofxCvGui::Widgets::Slider, this->checkerboardScale);
				inspector->add(slider);

				slider = MAKE(ofxCvGui::Widgets::Slider, this->checkerboardCornersX);
				slider->addIntValidator();
				inspector->add(slider);

				slider = MAKE(ofxCvGui::Widgets::Slider, this->checkerboardCornersY);
				slider->addIntValidator();
				inspector->add(slider);

				inspector->add(MAKE(ofxCvGui::Widgets::Slider, this->checkerboardPositionX));
				inspector->add(MAKE(ofxCvGui::Widgets::Slider, this->checkerboardPositionY));
				inspector->add(MAKE(ofxCvGui::Widgets::Slider, this->checkerboardBrightness));

				auto addButton = MAKE(ofxCvGui::Widgets::Button, "Add Capture", [this]() {
					try {
						this->addCapture();
					}
					OFXDIGITALEMULSION_CATCH_ALL_TO_ALERT
				}, ' ');
				addButton->setHeight(100.0f);
				inspector->add(addButton);

				auto calibrateButton = MAKE(ofxCvGui::Widgets::Button, "Calibrate", [this]() {
					try {
						this->calibrate();
					}
					OFXDIGITALEMULSION_CATCH_ALL_TO_ALERT
				}, OF_KEY_RETURN);
				calibrateButton->setHeight(100.0f);
				inspector->add(calibrateButton);
			}

			//----------
			void ProjectorFromKinectV2::drawWorld() {
				auto kinect = this->getInput<Item::KinectV2>();
				if (kinect) {
					ofPushStyle();
					ofSetColor(100);
					kinect->getDevice()->getDepthSource()->getMesh().drawVertices();
					ofPopStyle();
				}

				ofMesh preview;
				for (auto correspondence : this->correspondences) {
					preview.addVertex(correspondence.world);
					preview.addColor(ofColor(
						ofMap(correspondence.projector.x, -1, 1, 0, 255),
						ofMap(correspondence.projector.y, -1, 1, 0, 255),
						0));
				}
				glPushAttrib(GL_POINT_BIT);
				glEnable(GL_POINT_SMOOTH);
				glPointSize(10.0f);
				preview.drawVertices();
				glPopAttrib();
			}
		}
	}
}
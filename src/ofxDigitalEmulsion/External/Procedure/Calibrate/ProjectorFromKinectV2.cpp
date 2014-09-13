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
#include "ofxCvGui/Widgets/LiveValue.h"

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
				this->inputPins.push_back(MAKE(Pin<Item::Projector>));
				this->inputPins.push_back(MAKE(Pin<Device::ProjectorOutput>));

				this->checkerboardScale.set("Checkerboard Scale", 0.2f, 0.01f, 1.0f);
				this->checkerboardCornersX.set("Checkerboard Corners X", 5, 1, 10);
				this->checkerboardCornersY.set("Checkerboard Corners Y", 5, 1, 10);
				this->checkerboardPositionX.set("Checkerboard Position X", 0, -1, 1);
				this->checkerboardPositionY.set("Checkerboard Position Y", 0, -1, 1);
				this->checkerboardBrightness.set("Checkerboard Brightness", 0.5, 0, 1);
				this->initialLensOffset.set("Initial Lens Offset", 0.5f, -1.0f, 1.0f);

				this->error = 0.0f;
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
				auto colorView = MAKE(ofxCvGui::Panels::Draws, colorSource->getTextureReference());
				colorView->onDrawCropped += [this](ofxCvGui::Panels::BaseImage::DrawCroppedArguments & args) {
					ofPolyline previewLine;
					if (!this->previewCornerFinds.empty()) {
						ofCircle(this->previewCornerFinds.front(), 10.0f);
						for (const auto & previewCornerFind : this->previewCornerFinds) {
							previewLine.addVertex(previewCornerFind);
						}
					}
					ofPushStyle();
					ofSetColor(255, 0, 0);
					previewLine.draw();
					ofPopStyle();
				};
				view->add(colorView);

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
				ofxDigitalEmulsion::Utils::Serializable::serialize(this->initialLensOffset, json);

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

				json["error"] = this->error;
			}

			//----------
			void ProjectorFromKinectV2::deserialize(const Json::Value & json) {
				ofxDigitalEmulsion::Utils::Serializable::deserialize(this->checkerboardScale, json);
				ofxDigitalEmulsion::Utils::Serializable::deserialize(this->checkerboardCornersX, json);
				ofxDigitalEmulsion::Utils::Serializable::deserialize(this->checkerboardCornersY, json);
				ofxDigitalEmulsion::Utils::Serializable::deserialize(this->checkerboardPositionX, json);
				ofxDigitalEmulsion::Utils::Serializable::deserialize(this->checkerboardPositionY, json);
				ofxDigitalEmulsion::Utils::Serializable::deserialize(this->checkerboardBrightness, json);
				ofxDigitalEmulsion::Utils::Serializable::deserialize(this->initialLensOffset, json);

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

				this->error = json["error"].asFloat();
			}

			//----------
			void ProjectorFromKinectV2::addCapture() {
				this->throwIfMissingAnyConnection();

				auto kinectNode = this->getInput<Item::KinectV2>();
				auto kinectDevice = kinectNode->getDevice();
				auto colorPixels = kinectDevice->getColorSource()->getPixelsRef();
				auto colorImage = ofxCv::toCv(colorPixels);

				//flip the camera image
				cv::flip(colorImage, colorImage, 1);

				vector<ofVec2f> cameraPoints;
				bool success = ofxCv::findChessboardCornersPreTest(colorImage, cv::Size(this->checkerboardCornersX, this->checkerboardCornersY), toCv(cameraPoints));

				//flip the results back again
				int colorWidth = colorPixels.getWidth();
				for (auto & cameraPoint : cameraPoints) {
					cameraPoint.x = colorWidth - cameraPoint.x - 1;
				}

				this->previewCornerFinds.clear();

				if (success) {
					ofxDigitalEmulsion::Utils::playSuccessSound();
					auto cameraToWorldMap = kinectDevice->getDepthSource()->getColorToWorldMap();
					auto cameraToWorldPointer = (ofVec3f*) cameraToWorldMap.getPixels();
					auto cameraWidth = cameraToWorldMap.getWidth();
					auto checkerboardCorners = toOf(ofxCv::makeCheckerboardPoints(cv::Size(this->checkerboardCornersX, this->checkerboardCornersY), this->checkerboardScale, true));
					int pointIndex = 0;
					for (auto cameraPoint : cameraPoints) {
						this->previewCornerFinds.push_back(cameraPoint); 
						
						Correspondence correspondence;

						correspondence.world = cameraToWorldPointer[(int) cameraPoint.x + (int) cameraPoint.y * cameraWidth];
						correspondence.projector = (ofVec2f)checkerboardCorners[pointIndex] + ofVec2f(this->checkerboardPositionX, this->checkerboardPositionY);

						if (correspondence.world.z > 0.5f) {
							this->correspondences.push_back(correspondence);
						}

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

				vector<ofVec3f> worldPoints;
				vector<ofVec2f> projectorPoints;

				for (auto correpondence : this->correspondences) {
					worldPoints.push_back(correpondence.world);
					projectorPoints.push_back(correpondence.projector * ofVec2f(1,+1));
				}
				cv::Mat cameraMatrix, rotation, translation;
				this->error = ofxCv::calibrateProjector(cameraMatrix, rotation, translation,
					worldPoints, projectorPoints,
					this->getInput<Item::Projector>()->getWidth(), this->getInput<Item::Projector>()->getHeight(),
					this->initialLensOffset);
				this->getInput<Item::Projector>()->setExtrinsics(rotation, translation);
				this->getInput<Item::Projector>()->setIntrinsics(cameraMatrix);
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

				inspector->add(MAKE(ofxCvGui::Widgets::Button, "Clear correspondences", [this]() {
					this->correspondences.clear();
				}));

				inspector->add(MAKE(ofxCvGui::Widgets::Slider, this->initialLensOffset));
				auto calibrateButton = MAKE(ofxCvGui::Widgets::Button, "Calibrate", [this]() {
					try {
						this->calibrate();
					}
					OFXDIGITALEMULSION_CATCH_ALL_TO_ALERT
				}, OF_KEY_RETURN);
				calibrateButton->setHeight(100.0f);
				inspector->add(calibrateButton);
				inspector->add(MAKE(ofxCvGui::Widgets::LiveValue<float>, "Reprojection error", [this]() {
					return this->error;
				}));
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
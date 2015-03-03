#include "CameraFromKinectV2.h"

#include "../../../Item/Camera.h"
#include "../../../Item/Board.h"
#include "../../Item/KinectV2.h"

#include "../../../Exception.h"
#include "../../../Utils/Utils.h"

#include "ofxCvGui/Panels/Groups/Grid.h"
#include "ofxCvGui/Panels/Draws.h"
#include "ofxCvGui/Panels/World.h"

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
			CameraFromKinectV2::CameraFromKinectV2() {
				OFXDIGITALEMULSION_NODE_INIT_LISTENER;
			}

			//----------
			string CameraFromKinectV2::getTypeName() const {
				return "Procedure::Calibrate::CameraFromKinectV2";
			}

			//----------
			void CameraFromKinectV2::init() {
				OFXDIGITALEMULSION_NODE_UPDATE_LISTENER;
				OFXDIGITALEMULSION_NODE_SERIALIZATION_LISTENERS;
				OFXDIGITALEMULSION_NODE_INSPECTOR_LISTENER;

				this->addInput(MAKE(Pin<Item::KinectV2>));
				this->addInput(MAKE(Pin<Item::Camera>));
				this->addInput(MAKE(Pin<Item::Board>));

				this->usePreTest.set("Pre Test at low resolution", true);

				this->error = 0.0f;
				
				auto view = MAKE(ofxCvGui::Panels::Groups::Grid);
				auto worldView = MAKE(ofxCvGui::Panels::World);
				worldView->onDrawWorld += [this](ofCamera &) {
					this->drawWorld();
				};
				auto & camera = worldView->getCamera();
				camera.setPosition(-1, -1, 0);
				camera.lookAt(ofVec3f(0, 0, 3));
				view->add(worldView);

				auto kinectColorSource = this->getInput<Item::KinectV2>()->getDevice()->getColorSource();
				auto kinectColorView = MAKE(ofxCvGui::Panels::Draws, kinectColorSource->getTextureReference());
				kinectColorView->onDrawCropped += [this](ofxCvGui::Panels::BaseImage::DrawCroppedArguments & args) {
					ofPolyline previewLine;
					if (!this->previewCornerFindsKinect.empty()) {
						ofCircle(this->previewCornerFindsKinect.front(), 10.0f);
						for (const auto & previewCornerFind : this->previewCornerFindsKinect) {
							previewLine.addVertex(previewCornerFind);
						}
					}
					ofPushStyle();
					ofSetColor(255, 0, 0);
					previewLine.draw();
					ofPopStyle();
				};
				view->add(kinectColorView);

				auto cameraColorView = MAKE(ofxCvGui::Panels::Draws, this->getInput<Item::Camera>()->getGrabber()->getTextureReference());
				cameraColorView->onDrawCropped += [this](ofxCvGui::Panels::BaseImage::DrawCroppedArguments & args) {
					ofPolyline previewLine;
					if (!this->previewCornerFindsCamera.empty()) {
						ofCircle(this->previewCornerFindsCamera.front(), 10.0f);
						for (const auto & previewCornerFind : this->previewCornerFindsCamera) {
							previewLine.addVertex(previewCornerFind);
						}
					}
					ofPushStyle();
					ofSetColor(255, 0, 0);
					previewLine.draw();
					ofPopStyle();
				};
				view->add(cameraColorView);

				this->view = view;
			}

			//----------
			ofxCvGui::PanelPtr CameraFromKinectV2::getView() {
				return view;
			}

			//----------
			void CameraFromKinectV2::update() {

			}

			//----------
			void CameraFromKinectV2::serialize(Json::Value & json) {
				auto & jsonCorrespondences = json["correspondences"];
				int index = 0;
				for (const auto & correspondence : this->correspondences) {
					auto & jsonCorrespondence = jsonCorrespondences[index++];
					for (int i = 0; i < 3; i++) {
						jsonCorrespondence["world"][i] = correspondence.world[i];
					}
					for (int i = 0; i < 2; i++) {
						jsonCorrespondence["camera"][i] = correspondence.camera[i];
					}
				}

				json["error"] = this->error;
			}

			//----------
			void CameraFromKinectV2::deserialize(const Json::Value & json) {
				this->correspondences.clear();
				auto & jsonCorrespondences = json["correspondences"];
				for (const auto & jsonCorrespondence : jsonCorrespondences) {
					Correspondence correspondence;
					for (int i = 0; i < 3; i++) {
						correspondence.world[i] = jsonCorrespondence["world"][i].asFloat();
					}
					for (int i = 0; i < 2; i++) {
						correspondence.camera[i] = jsonCorrespondence["camera"][i].asFloat();
					}
					this->correspondences.push_back(correspondence);
				}

				this->error = json["error"].asFloat();
			}

			//----------
			void CameraFromKinectV2::addCapture() {
				this->throwIfMissingAnyConnection();

				auto kinectNode = this->getInput<Item::KinectV2>();
				auto kinectDevice = kinectNode->getDevice();
				auto kinectColorPixels = kinectDevice->getColorSource()->getPixelsRef();
				auto kinectColorImage = ofxCv::toCv(kinectColorPixels);

				auto cameraNode = this->getInput<Item::Camera>();
				auto cameraPixels = cameraNode->getFreshFrame();
				auto cameraColorImage = ofxCv::toCv(cameraPixels);
				auto cameraWidth = cameraPixels.getWidth();
				auto cameraHeight = cameraPixels.getHeight();

				auto checkerboardNode = this->getInput<Item::Board>();
				auto checkerboardSize = checkerboardNode->getSize();
				auto checkerboardObjectPoints = checkerboardNode->getObjectPoints();

				cv::cvtColor(cameraColorImage, cameraColorImage, CV_RGB2GRAY);

				//---
				//find the points in kinect space
				//---
				//

				//flip the kinect's image
				cv::flip(kinectColorImage, kinectColorImage, 1);

				vector<ofVec2f> kinectCameraPoints;
				bool foundInKinect;
				if (this->usePreTest)
				{
					foundInKinect = ofxCv::findChessboardCornersPreTest(kinectColorImage, checkerboardSize, toCv(kinectCameraPoints), 1024);
				} else {
					foundInKinect = ofxCv::findChessboardCorners(kinectColorImage, checkerboardSize, toCv(kinectCameraPoints));
				}
				

				//flip the results back again
				int colorWidth = kinectColorPixels.getWidth();
				for (auto & cameraPoint : kinectCameraPoints) {
					cameraPoint.x = colorWidth - cameraPoint.x - 1;
				}

				//
				//--

				//--
				//find the points in camera space
				//--
				//
				vector<ofVec2f> cameraPoints;
				bool foundInCamera;
				if (this->usePreTest)
				{
					foundInCamera = ofxCv::findChessboardCornersPreTest(cameraColorImage, checkerboardSize, toCv(cameraPoints), 1024);
				} else {
					foundInCamera = ofxCv::findChessboardCorners(cameraColorImage, checkerboardSize, toCv(cameraPoints));
				}
				//
				//--

				this->previewCornerFindsKinect.clear();
				this->previewCornerFindsCamera.clear();

				if (foundInKinect && foundInCamera) {
					ofxDigitalEmulsion::Utils::playSuccessSound();
					auto kinectCameraToWorldMap = kinectDevice->getDepthSource()->getColorToWorldMap();
					auto kinectCameraToWorldPointer = (ofVec3f*)kinectCameraToWorldMap.getPixels();
					auto kinectCameraWidth = kinectCameraToWorldMap.getWidth();
					int pointIndex = 0;
					for (int i = 0; i < cameraPoints.size(); i++) {
						this->previewCornerFindsKinect.push_back(kinectCameraPoints[i]);
						this->previewCornerFindsCamera.push_back(cameraPoints[i]);

						auto & kinectCameraPoint = kinectCameraPoints[i];
						auto & cameraPoint = cameraPoints[i];

						Correspondence correspondence;

						correspondence.world = kinectCameraToWorldPointer[(int)kinectCameraPoint.x + (int)kinectCameraPoint.y * kinectCameraWidth];
						correspondence.camera = ofVec2f(ofMap(cameraPoint.x, 0, cameraWidth, -1, 1),
							ofMap(cameraPoint.y, 0, cameraHeight, 1, -1));

						if (correspondence.world.z > 0.5f) {
							this->correspondences.push_back(correspondence);
						}

						pointIndex++;
					}
				} else {
					ofxDigitalEmulsion::Utils::playFailSound();
					OFXDIGITALEMULSION_ERROR << "Chesboard found in kinect [" << (foundInKinect ? "X" : " ") << "], camera [" << (foundInCamera ? "X" : " ") << "]";
				}
			}

			//----------
			void CameraFromKinectV2::calibrate() {
				this->throwIfMissingAnyConnection();

				vector<ofVec3f> worldPoints;
				vector<ofVec2f> projectorPoints;

				for (auto correpondence : this->correspondences) {
					worldPoints.push_back(correpondence.world * ofVec3f(1, 1, 1));
					projectorPoints.push_back(correpondence.camera * ofVec2f(1, -1));
				}
				cv::Mat cameraMatrix, rotation, translation;
				
				int flags = CV_CALIB_FIX_K1 | CV_CALIB_FIX_K2 | CV_CALIB_FIX_K3 | CV_CALIB_FIX_K4 | CV_CALIB_FIX_K5 | CV_CALIB_FIX_K6 | CV_CALIB_ZERO_TANGENT_DIST | CV_CALIB_USE_INTRINSIC_GUESS;
				flags |= CV_CALIB_FIX_PRINCIPAL_POINT | CV_CALIB_FIX_ASPECT_RATIO;
				
				auto width = this->getInput<Item::Camera>()->getWidth();
				auto height = this->getInput<Item::Camera>()->getHeight();

				this->error = ofxCv::calibrateProjector(cameraMatrix, rotation, translation,
					worldPoints, projectorPoints,
					width, height,
					0.0f, 1.0f, false, flags);
				this->getInput<Item::Camera>()->setExtrinsics(rotation, translation);
				this->getInput<Item::Camera>()->setIntrinsics(cameraMatrix, Mat::zeros(5, 1, CV_64F));

				auto viewMatrix = ofxCv::makeMatrix(rotation, translation);
				auto projectionMatrix = ofxCv::makeProjectionMatrix(cameraMatrix, cv::Size(width, height));

				const auto cameraName = this->getInput<Item::Camera>()->getName();
				ofstream file;
				file.open(ofToDataPath(cameraName + "View.mat").c_str(), ios::out | ios::binary);
				file.write((char*)viewMatrix.getPtr(), sizeof(float)* 16);
				file.close();
				file.open(ofToDataPath(cameraName + "Projection.mat").c_str(), ios::out | ios::binary);
				file.write((char*)projectionMatrix.getPtr(), sizeof(float)* 16);
				file.close();
			}

			//----------
			void CameraFromKinectV2::populateInspector(ofxCvGui::ElementGroupPtr inspector) {

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
				inspector->add(MAKE(ofxCvGui::Widgets::Toggle, this->usePreTest));
			}

			//----------
			void CameraFromKinectV2::drawWorld() {
				auto kinect = this->getInput<Item::KinectV2>();
				auto projector = this->getInput<Item::Camera>();

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
						ofMap(correspondence.camera.x, -1, 1, 0, 255),
						ofMap(correspondence.camera.y, -1, 1, 0, 255),
						0));
				}
				glPushAttrib(GL_POINT_BIT);
				glEnable(GL_POINT_SMOOTH);
				glPointSize(10.0f);
				preview.drawVertices();
				glPopAttrib();

				if (projector) {
					projector->drawWorld();
				}
			}
		}
	}
}
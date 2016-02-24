#include "pch_Plugin_KinectForWindows2.h"
#include "CameraFromKinectV2.h"

#include "ofxRulr/Nodes/Item/Camera.h"
#include "ofxRulr/Nodes/Item/Board.h"
#include "ofxRulr/Nodes/Item/KinectV2.h"

#include "ofxRulr/Exception.h"
#include "ofxRulr/Utils/ScopedProcess.h"

#include "ofxCvGui/Panels/Groups/Grid.h"
#include "ofxCvGui/Panels/Draws.h"
#include "ofxCvGui/Panels/World.h"

#include "ofxCvGui/Widgets/Slider.h"
#include "ofxCvGui/Widgets/Button.h"
#include "ofxCvGui/Widgets/LiveValue.h"

#include "ofxCvMin.h"

using namespace ofxRulr::Graph;
using namespace ofxCvGui;

using namespace ofxCv;
using namespace cv;

namespace ofxRulr {
	namespace Nodes {
		namespace Procedure {
			namespace Calibrate {
				//----------
				CameraFromKinectV2::CameraFromKinectV2() {
					RULR_NODE_INIT_LISTENER;
				}

				//----------
				string CameraFromKinectV2::getTypeName() const {
					return "Procedure::Calibrate::CameraFromKinectV2";
				}

				//----------
				void CameraFromKinectV2::init() {
					RULR_NODE_UPDATE_LISTENER;
					RULR_NODE_SERIALIZATION_LISTENERS;
					RULR_NODE_INSPECTOR_LISTENER;

					this->addInput(MAKE(Pin<Item::KinectV2>));
					this->addInput(MAKE(Pin<Item::Camera>));
					this->addInput(MAKE(Pin<Item::Board>));

					this->usePreTest.set("Pre Test at low resolution", true);

					this->error = 0.0f;

					this->view = MAKE(ofxCvGui::Panels::Groups::Grid);
					this->onAnyInputConnectionChanged += [this]() {
						this->rebuildView();
					};
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
						jsonCorrespondence["kinectObject"] << correspondence.kinectObject;
						jsonCorrespondence["camera"] << correspondence.camera;
						jsonCorrespondence["cameraNormalized"] << correspondence.cameraNormalized;
					}

					json["error"] = this->error;
				}

				//----------
				void CameraFromKinectV2::deserialize(const Json::Value & json) {
					this->correspondences.clear();
					auto & jsonCorrespondences = json["correspondences"];
					for (const auto & jsonCorrespondence : jsonCorrespondences) {
						Correspondence correspondence;
						jsonCorrespondence["kinectObject"] >> correspondence.kinectObject;
						jsonCorrespondence["camera"] >> correspondence.camera;
						jsonCorrespondence["cameraNormalized"] >> correspondence.cameraNormalized;
						this->correspondences.push_back(correspondence);
					}

					this->error = json["error"].asFloat();
				}

				//----------
				void CameraFromKinectV2::addCapture() {
					this->throwIfMissingAnyConnection();

					auto kinectNode = this->getInput<Item::KinectV2>();
					auto kinectDevice = kinectNode->getDevice();
					auto kinectColorPixels = kinectDevice->getColorSource()->getPixels();
					auto kinectColorImage = ofxCv::toCv(kinectColorPixels);
					const auto kinectTransform = kinectNode->getTransform();

					auto cameraNode = this->getInput<Item::Camera>();
					auto cameraFrame = cameraNode->getFreshFrame();
					auto & cameraPixels = cameraFrame->getPixels();
					auto cameraColorImage = ofxCv::toCv(cameraPixels);
					auto cameraWidth = cameraPixels.getWidth();
					auto cameraHeight = cameraPixels.getHeight();

					auto checkerboardNode = this->getInput<Item::Board>();
					auto checkerboardSize = checkerboardNode->getSize();
					auto checkerboardObjectPoints = checkerboardNode->getObjectPoints();

					//convert to grayscale
					if (cameraColorImage.channels() == 3) {
						cv::cvtColor(cameraColorImage, cameraColorImage, CV_RGB2GRAY);
					}

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
					}
					else {
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
					}
					else {
						foundInCamera = ofxCv::findChessboardCorners(cameraColorImage, checkerboardSize, toCv(cameraPoints));
					}
					//
					//--

					this->previewCornerFindsKinect.clear();
					this->previewCornerFindsCamera.clear();

					if (!foundInKinect || !foundInCamera) {
						stringstream error;
						error << "Chesboard found in kinect [" << (foundInKinect ? "X" : " ") << "], camera [" << (foundInCamera ? "X" : " ") << "]";
						throw(ofxRulr::Exception(error.str()));
					}

					ofFloatPixels kinectCameraToWorldMap;
					kinectDevice->getDepthSource()->getWorldInColorFrame(kinectCameraToWorldMap);

					auto kinectCameraToWorldPointer = (ofVec3f*)kinectCameraToWorldMap.getData();
					auto kinectCameraWidth = kinectCameraToWorldMap.getWidth();
					int pointIndex = 0;
					for (int i = 0; i < cameraPoints.size(); i++) {
						this->previewCornerFindsKinect.push_back(kinectCameraPoints[i]);
						this->previewCornerFindsCamera.push_back(cameraPoints[i]);

						auto & kinectCameraPoint = kinectCameraPoints[i];
						auto & cameraPoint = cameraPoints[i];

						Correspondence correspondence;

						correspondence.kinectObject = kinectCameraToWorldPointer[(int)kinectCameraPoint.x + (int)kinectCameraPoint.y * kinectCameraWidth];
						correspondence.camera = cameraPoint;
						correspondence.cameraNormalized = ofVec2f(ofMap(cameraPoint.x, 0, cameraWidth, 0, 1),
							ofMap(cameraPoint.y, 0, cameraHeight, 0, 1));

						if (correspondence.kinectObject.z > 0.5f) {
							this->correspondences.push_back(correspondence);
						}

						pointIndex++;
					}
				}

				//----------
				void CameraFromKinectV2::calibrate() {
					this->throwIfMissingAnyConnection();

					auto camera = this->getInput<Item::Camera>();
					auto kinect = this->getInput<Item::KinectV2>();
					auto kinectTransform = kinect->getTransform();

					vector<ofVec3f> worldPoints;
					vector<ofVec2f> cameraPoints;

					for (auto correpondence : this->correspondences) {
						worldPoints.push_back(correpondence.kinectObject * kinectTransform);
						cameraPoints.push_back(correpondence.camera);
					}

					const auto worldPointsRows = vector<vector<cv::Point3f> >(1, ofxCv::toCv(worldPoints));
					const auto cameraPointsRows = vector<vector<cv::Point2f> >(1, ofxCv::toCv(cameraPoints));

					auto cameraMatrix = camera->getCameraMatrix();
					auto distortion = camera->getDistortionCoefficients();

					vector<cv::Mat> rotations, translations;

					//fix intrinsics
					int flags = CV_CALIB_FIX_K1 | CV_CALIB_FIX_K2 | CV_CALIB_FIX_K3 | CV_CALIB_FIX_K4 | CV_CALIB_FIX_K5 | CV_CALIB_FIX_K6 | CV_CALIB_ZERO_TANGENT_DIST | CV_CALIB_USE_INTRINSIC_GUESS;
					flags |= CV_CALIB_FIX_PRINCIPAL_POINT | CV_CALIB_FIX_ASPECT_RATIO | CV_CALIB_FIX_FOCAL_LENGTH;

					auto width = this->getInput<Item::Camera>()->getWidth();
					auto height = this->getInput<Item::Camera>()->getHeight();

					this->error = cv::calibrateCamera(worldPointsRows, cameraPointsRows, camera->getSize(), cameraMatrix, distortion, rotations, translations, flags);

					camera->setExtrinsics(rotations[0], translations[0], false);

					//camera->setIntrinsics(cameraMatrix, distortion); <-- intrinsics shouldn't change
				}

				//----------
				void CameraFromKinectV2::populateInspector(ofxCvGui::InspectArguments & inspectArgs) {
					auto inspector = inspectArgs.inspector;

					inspector->add(new ofxCvGui::Widgets::LiveValue<int>("Correspondences found", [this]() {
						return (int) this->correspondences.size();
					}));

					auto addButton = new ofxCvGui::Widgets::Button("Add Capture", [this]() {
						try {
							Utils::ScopedProcess scopedProcess("CameraFromKinectV2 - addCapture");
							this->addCapture();
							scopedProcess.end();
						}
						RULR_CATCH_ALL_TO_ALERT
					}, ' ');
					addButton->setHeight(100.0f);
					inspector->add(addButton);

					inspector->add(MAKE(ofxCvGui::Widgets::Button, "Clear correspondences", [this]() {
						this->correspondences.clear();
					}));

					auto calibrateButton = new ofxCvGui::Widgets::Button("Calibrate", [this]() {
						try {
							this->calibrate();
						}
						RULR_CATCH_ALL_TO_ALERT
					}, OF_KEY_RETURN);
					calibrateButton->setHeight(100.0f);
					inspector->add(calibrateButton);
					inspector->add(new ofxCvGui::Widgets::LiveValue<float>("Reprojection error", [this]() {
						return this->error;
					}));
					inspector->add(new ofxCvGui::Widgets::Toggle(this->usePreTest));
				}

				//----------
				void CameraFromKinectV2::drawWorld() {
					auto kinect = this->getInput<Item::KinectV2>();
					if (kinect) {
						auto kinectTransform = kinect->getTransform();

						ofMesh preview;
						for (auto correspondence : this->correspondences) {
							preview.addVertex(correspondence.kinectObject * kinectTransform);
							preview.addColor(ofColor(
								correspondence.cameraNormalized.x * 255.0f,
								correspondence.cameraNormalized.y * 255.0f,
								0));
						}
						glPushAttrib(GL_POINT_BIT);
						glEnable(GL_POINT_SMOOTH);
						glPointSize(10.0f);
						preview.drawVertices();
						glPopAttrib();
					}
				}

				//----------
				void CameraFromKinectV2::rebuildView() {
					this->view->clear();

					auto kinect = this->getInput<Item::KinectV2>();
					auto camera = this->getInput<Item::Camera>();

					if (kinect && camera) {
						auto kinectColorSource = kinect->getDevice()->getColorSource();
						auto kinectColorView = MAKE(ofxCvGui::Panels::Draws, kinectColorSource->getTexture());
						kinectColorView->onDrawCropped += [this](ofxCvGui::Panels::BaseImage::DrawCroppedArguments & args) {
							ofPolyline previewLine;
							if (!this->previewCornerFindsKinect.empty()) {
								ofDrawCircle(this->previewCornerFindsKinect.front(), 10.0f);
								for (const auto & previewCornerFind : this->previewCornerFindsKinect) {
									previewLine.addVertex(previewCornerFind);
								}
							}
							ofPushStyle();
							ofSetColor(255, 0, 0);
							previewLine.draw();
							ofPopStyle();
						};
						kinectColorView->setCaption("Kinect RGB");
						this->view->add(kinectColorView);

						auto cameraColorView = MAKE(ofxCvGui::Panels::Draws, camera->getGrabber()->getTexture());
						cameraColorView->onDrawCropped += [this](ofxCvGui::Panels::BaseImage::DrawCroppedArguments & args) {
							ofPolyline previewLine;
							if (!this->previewCornerFindsCamera.empty()) {
								ofDrawCircle(this->previewCornerFindsCamera.front(), 10.0f);
								for (const auto & previewCornerFind : this->previewCornerFindsCamera) {
									previewLine.addVertex(previewCornerFind);
								}
							}
							ofPushStyle();
							ofSetColor(255, 0, 0);
							previewLine.draw();
							ofPopStyle();
						};
						cameraColorView->setCaption("Camera");
						this->view->add(cameraColorView);
					}
				}
			}
		}
	}
}
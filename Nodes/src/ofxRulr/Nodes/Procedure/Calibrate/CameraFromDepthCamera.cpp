#include "pch_RulrNodes.h"
#include "CameraFromDepthCamera.h"

#include "ofxRulr/Nodes/Item/Camera.h"
#include "ofxRulr/Nodes/Item/Board.h"
#include "ofxRulr/Nodes/Item/IDepthCamera.h"

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
				CameraFromDepthCamera::CameraFromDepthCamera() {
					RULR_NODE_INIT_LISTENER;
				}
				
				//----------
				string CameraFromDepthCamera::getTypeName() const {
					return "Procedure::Calibrate::CameraFromDepthCamera";
				}
				
				//----------
				void CameraFromDepthCamera::init() {
					RULR_NODE_UPDATE_LISTENER;
					RULR_NODE_DRAW_WORLD_LISTENER;
					RULR_NODE_SERIALIZATION_LISTENERS;
					RULR_NODE_INSPECTOR_LISTENER;
					
					this->addInput(MAKE(Pin<Item::IDepthCamera>));
					this->addInput(MAKE(Pin<Item::Camera>));
					this->addInput(MAKE(Pin<Item::Board>));
					
					this->usePreTest.set("Pre Test at low resolution", true);
					
					this->error = 0.0f;
					
					this->view = MAKE(ofxCvGui::Panels::Groups::Grid);
					this->onAnyInputConnectionChanged += [this]() {
						try {
							this->rebuildView();
						}
						RULR_CATCH_ALL_TO_ERROR;
					};
				}
				
				//----------
				ofxCvGui::PanelPtr CameraFromDepthCamera::getPanel() {
					return view;
				}
				
				//----------
				void CameraFromDepthCamera::update() {
					
				}
				
				//----------
				void CameraFromDepthCamera::serialize(Json::Value & json) {
					auto & jsonCorrespondences = json["correspondences"];
					int index = 0;
					for (const auto & correspondence : this->correspondences) {
						auto & jsonCorrespondence = jsonCorrespondences[index++];
						jsonCorrespondence["kinectObject"] << correspondence.depthCameraObject;
						jsonCorrespondence["camera"] << correspondence.camera;
						jsonCorrespondence["cameraNormalized"] << correspondence.cameraNormalized;
					}
					
					json["error"] = this->error;
				}
				
				//----------
				void CameraFromDepthCamera::deserialize(const Json::Value & json) {
					this->correspondences.clear();
					auto & jsonCorrespondences = json["correspondences"];
					for (const auto & jsonCorrespondence : jsonCorrespondences) {
						Correspondence correspondence;
						jsonCorrespondence["kinectObject"] >> correspondence.depthCameraObject;
						jsonCorrespondence["camera"] >> correspondence.camera;
						jsonCorrespondence["cameraNormalized"] >> correspondence.cameraNormalized;
						this->correspondences.push_back(correspondence);
					}
					
					this->error = json["error"].asFloat();
				}
				
				//----------
				void CameraFromDepthCamera::addCapture() {
					this->throwIfMissingAnyConnection();
					
					auto depthCameraNode = this->getInput<Item::IDepthCamera>();
					auto irPixels = depthCameraNode->getIRPixels();
					if(!irPixels) {
						throw(Exception("Cannot get IR pixels"));
					}
					auto irMat = toCv(*irPixels);
					
					const auto depthCameraTransform = depthCameraNode->getTransform();
					
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
					//find the points in depth camera space
					//---
					//
					
					//flip the depth camera's image
					cv::flip(irMat, irMat, 1);
					
					vector<ofVec2f> irPoints;
					bool foundInDepthCamera;
					if (this->usePreTest)
					{
						foundInDepthCamera = ofxCv::findChessboardCornersPreTest(irMat, checkerboardSize, toCv(irPoints), 1024);
					}
					else {
						foundInDepthCamera = ofxCv::findChessboardCorners(irMat, checkerboardSize, toCv(irPoints));
					}
					
					
					//flip the results back again
					int irWidth = irPixels->getWidth();
					for (auto & irPoint : irPoints) {
						irPoint.x = irWidth - irPoint.x - 1;
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
					
					this->previewCornerFindsDepthCamera.clear();
					this->previewCornerFindsCamera.clear();
					
					//fill correspondences
					if (!foundInDepthCamera || !foundInCamera) {
						stringstream errorstring;
						errorstring << "Chesboard found in kinect [" << (foundInDepthCamera ? "X" : " ") << "], camera [" << (foundInCamera ? "X" : " ") << "]";
						throw(ofxRulr::Exception(errorstring.str()));
					}
					
					auto world = depthCameraNode->getWorldPixels();
					auto depthMapWidth = irPixels->getWidth();
					if(!world) {
						throw(Exception("Cannot get world map"));
					}
					auto worldVectors = (ofVec3f*) world->getData();
					int pointIndex = 0;
					for (int i = 0; i < cameraPoints.size(); i++) {
						this->previewCornerFindsDepthCamera.push_back(irPoints[i]);
						this->previewCornerFindsCamera.push_back(cameraPoints[i]);
						
						auto & irPoint = irPoints[i];
						auto & cameraPoint = cameraPoints[i];
						
						Correspondence correspondence;
						
						correspondence.depthCameraObject = worldVectors[(int)irPoint.x + (int)irPoint.y * depthMapWidth];
						correspondence.camera = cameraPoint;
						correspondence.cameraNormalized = ofVec2f(ofMap(cameraPoint.x, 0, cameraWidth, 0, 1),
																  ofMap(cameraPoint.y, 0, cameraHeight, 0, 1));
						
						if (correspondence.depthCameraObject.z > 0.1f) {
							this->correspondences.push_back(correspondence);
						}
						
						pointIndex++;
					}
				}
				
				//----------
				void CameraFromDepthCamera::calibrate() {
					this->throwIfMissingAnyConnection();
					
					auto camera = this->getInput<Item::Camera>();
					auto depthCamera = this->getInput<Item::IDepthCamera>();
					auto depthCameraTransform = depthCamera->getTransform();
					
					vector<ofVec3f> worldPoints;
					vector<ofVec2f> cameraPoints;
					
					for (auto correpondence : this->correspondences) {
						worldPoints.push_back(correpondence.depthCameraObject * depthCameraTransform);
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
				void CameraFromDepthCamera::populateInspector(ofxCvGui::InspectArguments & inspectArguments) {
					auto inspector = inspectArguments.inspector;
					
					inspector->add(new ofxCvGui::Widgets::LiveValue<int>("Correspondences found", [this]() {
						return (int) this->correspondences.size();
					}));
					auto addButton = MAKE(ofxCvGui::Widgets::Button, "Add Capture", [this]() {
						try {
							Utils::ScopedProcess scopedProcess("CameraFromDepthCamera - addCapture");
							this->addCapture();
							scopedProcess.end();
						}
						RULR_CATCH_ALL_TO_ALERT
					}, ' ');
					addButton->setHeight(100.0f);
					inspector->add(addButton);
					
					inspector->addButton("Clear correspondences", [this]() {
						this->correspondences.clear();
					});
					
					auto calibrateButton = MAKE(ofxCvGui::Widgets::Button, "Calibrate", [this]() {
						try {
							this->calibrate();
						}
						RULR_CATCH_ALL_TO_ALERT
					}, OF_KEY_RETURN);
					calibrateButton->setHeight(100.0f);
					inspector->add(calibrateButton);
					inspector->addLiveValue<float>("Reprojection error", [this]() {
						return this->error;
					});
					inspector->addToggle(this->usePreTest);
				}
				
				//----------
				void CameraFromDepthCamera::drawWorld() {
					auto depthCamera = this->getInput<Item::IDepthCamera>();
					if (depthCamera) {
						auto depthCameraTransform = depthCamera->getTransform();
						
						ofMesh preview;
						for (auto correspondence : this->correspondences) {
							preview.addVertex(correspondence.depthCameraObject);
							preview.addColor(ofColor(
													 correspondence.cameraNormalized.x * 255.0f,
													 correspondence.cameraNormalized.y * 255.0f,
													 0));
						}
						
						ofPushMatrix();
						{
							ofMultMatrix(depthCameraTransform);
							
							Utils::Graphics::pushPointSize(10.0f);
							{
								preview.drawVertices();
							}
							Utils::Graphics::popPointSize();
							
						}
						ofPopMatrix();
					}
				}
				
				//----------
				void CameraFromDepthCamera::rebuildView() {
					this->view->clear();
					
					auto depthCamera = this->getInput<Item::IDepthCamera>();
					auto camera = this->getInput<Item::Camera>();
					
					if (depthCamera && camera) {
						auto depthCameraIRTexture = depthCamera->getIRTexture();
						if(!depthCameraIRTexture) {
							throw(Exception("Needs IR texture"));
						}
						auto irView = make_shared<Panels::Draws>(*depthCameraIRTexture);
						irView->onDrawImage += [this](ofxCvGui::DrawImageArguments & args) {
							ofPolyline previewLine;
							if (!this->previewCornerFindsDepthCamera.empty()) {
								ofDrawCircle(this->previewCornerFindsDepthCamera.front(), 10.0f);
								for (const auto & previewCornerFind : this->previewCornerFindsDepthCamera) {
									previewLine.addVertex(previewCornerFind);
								}
							}
							ofPushStyle();
							ofSetColor(255, 0, 0);
							previewLine.draw();
							ofPopStyle();
						};
						irView->setCaption("IR");
						this->view->add(irView);
						
						auto cameraColorView = MAKE(ofxCvGui::Panels::Draws, camera->getGrabber()->getTexture());
						cameraColorView->onDrawImage += [this](ofxCvGui::DrawImageArguments & args) {
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
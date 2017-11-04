#include "pch_Plugin_KinectForWindows2.h"
#include "CameraFromKinectV2.h"

#include "ofxRulr/Nodes/Item/Camera.h"
#include "ofxRulr/Nodes/Item/AbstractBoard.h"
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
					this->addInput(MAKE(Pin<Item::AbstractBoard>));

					this->error = 0.0f;

					this->view = MAKE(ofxCvGui::Panels::Groups::Grid);
					this->onAnyInputConnectionChanged += [this]() {
						this->rebuildView();
					};

					this->manageParameters(this->parameters);
				}

				//----------
				ofxCvGui::PanelPtr CameraFromKinectV2::getPanel() {
					return view;
				}

				//----------
				void CameraFromKinectV2::update() {

				}

				//----------
				void CameraFromKinectV2::serialize(Json::Value & json) {
					this->captures.serialize(json);
					json["error"] = this->error;
				}

				//----------
				void CameraFromKinectV2::deserialize(const Json::Value & json) {
					this->captures.deserialize(json);
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

					auto boardNode = this->getInput<Item::AbstractBoard>();

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

					vector<cv::Point2f> kinectCameraPoints;
					vector<cv::Point3f> kinectBoardObjectPoints;
					bool foundInKinect = boardNode->findBoard(kinectColorImage, kinectCameraPoints, kinectBoardObjectPoints, this->parameters.findBoardMode);
					if (kinectCameraPoints.size() != kinectBoardObjectPoints.size()) {
						throw(ofxRulr::Exception("This node currently doesn't support finding a sub-set of board image points"));
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
					vector<cv::Point2f> cameraPoints;
					vector<cv::Point3f> cameraObjectPoints;
					bool foundInCamera = boardNode->findBoard(cameraColorImage, cameraPoints, cameraObjectPoints, this->parameters.findBoardMode);
					if (cameraPoints.size() != cameraObjectPoints.size()) {
						throw(ofxRulr::Exception("This node currently doesn't support finding a sub-set of board image points"));
					}
					//
					//--

					if (foundInKinect) {
						lastTimeCheckerboardSeenInKinect = chrono::system_clock::now();
					}
					if (foundInCamera) {
						lastTimeCheckerboardSeenInCamera = chrono::system_clock::now();
					}

					if (!foundInKinect || !foundInCamera) {
						stringstream error;
						error << "Board found in kinect [" << (foundInKinect ? "X" : " ") << "], camera [" << (foundInCamera ? "X" : " ") << "]";
						throw(ofxRulr::Exception(error.str()));
					}

					ofFloatPixels kinectCameraToWorldMap;
					kinectDevice->getDepthSource()->getWorldInColorFrame(kinectCameraToWorldMap);

					auto kinectCameraToWorldPointer = (ofVec3f*)kinectCameraToWorldMap.getData();
					auto kinectCameraWidth = kinectCameraToWorldMap.getWidth();
					int pointIndex = 0;

					auto capture = make_shared<Capture>();
					capture->kinectImageSpace = kinectCameraPoints;
					capture->cameraImageSpace = cameraPoints;

					for (int i = 0; i < cameraPoints.size(); i++) {
						capture->kinectObjectSpace.push_back((const cv::Point3f &) kinectCameraToWorldPointer[(int)kinectCameraPoints[i].x + (int)kinectCameraPoints[i].y * kinectCameraWidth]);
						capture->cameraNormalizedSpace.emplace_back(ofMap(cameraPoints[i].x, 0, cameraWidth, 0, 1),
							ofMap(cameraPoints[i].y, 0, cameraHeight, 0, 1));
					}


					//remove any points where kinect reading of z is zero
					auto kinectImageSpaceIt = capture->kinectImageSpace.begin();
					auto kinectObjectSpaceIt = capture->kinectObjectSpace.begin();
					auto cameraImageSpaceIt = capture->cameraImageSpace.begin();
					auto cameraNormalizedSpaceIt = capture->cameraNormalizedSpace.begin();
					for (int i = 0; i < cameraPoints.size(); i++) {
						if (kinectObjectSpaceIt->z == 0) {
							kinectImageSpaceIt = capture->kinectImageSpace.erase(kinectImageSpaceIt);
							kinectObjectSpaceIt = capture->kinectObjectSpace.erase(kinectObjectSpaceIt);
							cameraImageSpaceIt = capture->cameraImageSpace.erase(cameraImageSpaceIt);
							cameraNormalizedSpaceIt = capture->cameraNormalizedSpace.erase(cameraNormalizedSpaceIt);
						}
						else {
							kinectImageSpaceIt++;
							kinectObjectSpaceIt++;
							cameraImageSpaceIt++;
							cameraNormalizedSpaceIt++;
						}
					}
					this->captures.add(capture);
				}

				//----------
				void CameraFromKinectV2::calibrate() {
					this->throwIfMissingAnyConnection();

					auto camera = this->getInput<Item::Camera>();
					auto kinect = this->getInput<Item::KinectV2>();
					auto kinectTransform = kinect->getTransform();

					vector<ofVec3f> worldPoints;
					vector<ofVec2f> cameraPoints;

					auto captures = this->captures.getSelection();
					for (auto capture : captures) {
						for (auto objectPoint : capture->kinectObjectSpace) {
							worldPoints.push_back(ofxCv::toOf(objectPoint) * kinectTransform);
						}
						for (auto cameraImageSpacePoint : capture->cameraImageSpace) {
							cameraPoints.push_back(ofxCv::toOf(cameraImageSpacePoint));
						}
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

					this->captures.populateWidgets(inspector);

					auto addButton = new ofxCvGui::Widgets::Button("Add Capture", [this]() {
						try {
							Utils::ScopedProcess scopedProcess("CameraFromKinectV2 - addCapture");
							this->addCapture();
							scopedProcess.end();
						}
						RULR_CATCH_ALL_TO_ERROR
					}, ' ');
					addButton->setHeight(100.0f);
					inspector->add(addButton);

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
				}

				//----------
				void CameraFromKinectV2::drawWorldStage() {
					auto kinect = this->getInput<Item::KinectV2>();
					if (kinect) {
						auto kinectTransform = kinect->getTransform();

						auto captures = this->captures.getSelection();
						for (auto capture : captures) {
							capture->drawObjectSpace();
						}
					}
				}

				//----------
				void CameraFromKinectV2::rebuildView() {
					this->view->clear();

					auto kinect = this->getInput<Item::KinectV2>();
					auto camera = this->getInput<Item::Camera>();

					if (kinect && camera) {
						auto drawSuccessIndicator = [](const ofRectangle & bounds, const chrono::system_clock::time_point & lastSuccess) {
							auto timeSinceLastSuccess = chrono::system_clock::now() - lastSuccess;
							auto millis = chrono::duration_cast<chrono::milliseconds>(timeSinceLastSuccess).count();
							if (millis < 3000) {
								ofPushStyle();
								{
									ofEnableBlendMode(ofBlendMode::OF_BLENDMODE_ADD);
									ofSetColor(0, ofMap(millis, 0, 3000, 255, 0), 0);
									ofDrawRectangle(bounds);
								}
								ofPopStyle();
							}
						};

						auto kinectColorSource = kinect->getDevice()->getColorSource();
						auto kinectColorView = MAKE(ofxCvGui::Panels::Draws, kinectColorSource->getTexture());
						kinectColorView->onDrawImage += [this](ofxCvGui::DrawImageArguments & args) {
							auto captures = this->captures.getSelection();
							for (auto capture : captures) {
								capture->drawKinectImageSpace();
							}
						};
						kinectColorView->onDraw += [this, drawSuccessIndicator] (ofxCvGui::DrawArguments & args) {
							drawSuccessIndicator(args.localBounds, this->lastTimeCheckerboardSeenInKinect);
						};
						kinectColorView->setCaption("Kinect RGB");
						this->view->add(kinectColorView);

						auto cameraColorView = MAKE(ofxCvGui::Panels::Draws, camera->getGrabber()->getTexture());
						cameraColorView->onDrawImage += [this, drawSuccessIndicator](ofxCvGui::DrawImageArguments & args) {
							auto captures = this->captures.getSelection();
							for (auto capture : captures) {
								capture->drawKinectImageSpace();
							}
						};
						cameraColorView->onDraw += [this, drawSuccessIndicator](ofxCvGui::DrawArguments & args) {
							drawSuccessIndicator(args.localBounds, this->lastTimeCheckerboardSeenInCamera);
						};
						cameraColorView->setCaption("Camera");
						this->view->add(cameraColorView);
					}
				}

				//----------
				CameraFromKinectV2::Capture::Capture() {
					RULR_SERIALIZE_LISTENERS;
				}

				//----------
				std::string CameraFromKinectV2::Capture::getDisplayString() const {
					stringstream ss;
					ss << kinectObjectSpace.size() << " points found.";
					return ss.str();
				}

				//----------
				void CameraFromKinectV2::Capture::serialize(Json::Value & json) {
					json["kinectObjectSpace"] << ofxCv::toOf(this->kinectImageSpace);
					json["kinectObjectSpace"] << ofxCv::toOf(this->kinectObjectSpace);
					json["cameraImageSpace"] << ofxCv::toOf(this->cameraImageSpace);
					json["cameraNormalizedSpace"] << ofxCv::toOf(this->cameraNormalizedSpace);
				}

				//----------
 				void CameraFromKinectV2::Capture::deserialize(const Json::Value & json) {
					json["kinectObjectSpace"] >> ofxCv::toOf(this->kinectImageSpace);
					json["kinectObjectSpace"] >> ofxCv::toOf(this->kinectObjectSpace);
 					json["cameraImageSpace"] >> ofxCv::toOf(this->cameraImageSpace);
 					json["cameraNormalizedSpace"] >> ofxCv::toOf(this->cameraNormalizedSpace);
				}

				//----------
				void CameraFromKinectV2::Capture::drawObjectSpace() {
					ofPushStyle();
					{
						ofSetColor(this->color);
						ofxCv::drawCorners(this->kinectObjectSpace, true);
					}
					ofPopStyle();
				}

				//----------
				void CameraFromKinectV2::Capture::drawKinectImageSpace() {
					ofPushStyle();
					{
						ofSetColor(this->color);
						ofxCv::drawCorners(this->kinectImageSpace, true);
					}
					ofPopStyle();
				}
			}
		}
	}
}
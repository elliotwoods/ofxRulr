#include "pch_RulrNodes.h"
#include "ProjectorFromStereoAndHelperCamera.h"

#include "StereoCalibrate.h"
#include "ofxRulr/Nodes/Procedure/Scan/Graycode.h"
#include "ofxRulr/Nodes/Item/Camera.h"
#include "ofxRulr/Nodes/Item/Projector.h"
#include "ofxRulr/Nodes/Item/Board.h"
#include "ofxRulr/Utils/ThreadPool.h"

namespace ofxRulr {
	namespace Nodes {
		namespace Procedure {
			namespace Calibrate {
#pragma mark Capture

				//----------
				ProjectorFromStereoAndHelperCamera::Capture::Capture() {
					RULR_NODE_SERIALIZATION_LISTENERS;
				}

				//----------
				void ProjectorFromStereoAndHelperCamera::Capture::serialize(Json::Value & json) {
					json["worldSpacePoints"] << this->worldSpacePoints;
					json["imageSpacePoints"] << this->imageSpacePoints;

				}

				//----------
				void ProjectorFromStereoAndHelperCamera::Capture::deserialize(const Json::Value & json) {
					json["worldSpacePoints"] >> this->worldSpacePoints;
					json["imageSpacePoints"] >> this->imageSpacePoints;
				}

				//----------
				std::string ProjectorFromStereoAndHelperCamera::Capture::getDisplayString() const {
					stringstream ss;
					ss << this->worldSpacePoints.size() << " points";
					return ss.str();
				}

#pragma mark ProjectorFromStereoAndHelperCamera
				//----------
				ProjectorFromStereoAndHelperCamera::ProjectorFromStereoAndHelperCamera() {
					RULR_NODE_INIT_LISTENER;
				}

				//----------
				std::string ProjectorFromStereoAndHelperCamera::getTypeName() const {
					return "Procedure::Calibrate::ProjectorFromStereoAndHelperCamera";
				}

				//----------
				void ProjectorFromStereoAndHelperCamera::init() {
					RULR_NODE_UPDATE_LISTENER;
					RULR_NODE_SERIALIZATION_LISTENERS;
					RULR_NODE_INSPECTOR_LISTENER;

					this->addInput<StereoCalibrate>();
					this->addInput<Scan::Graycode>();
					this->addInput<Item::Board>();

					auto videoOutputPin = this->addInput<System::VideoOutput>();
					videoOutputListener = make_unique<Utils::VideoOutputListener>(videoOutputPin, [this](const ofRectangle & bounds) {
						this->drawOnOutput(bounds);
					});

					this->manageParameters(this->parameters);
				}

				//----------
				void ProjectorFromStereoAndHelperCamera::update() {

				}

				//----------
				ofxCvGui::PanelPtr ProjectorFromStereoAndHelperCamera::getPanel() {
					return this->panel;
				}

				//----------
				void ProjectorFromStereoAndHelperCamera::addCapture() {
					this->throwIfMissingAnyConnection();

					auto stereoCalibrateNode = this->getInput<StereoCalibrate>();
					auto graycodeNode = this->getInput<Scan::Graycode>();
					auto boardNode = this->getInput<Item::Board>();

					Utils::ThreadPool threadPool(2, 5);

					//find the checkerboard corners in stereo space
					// (this would ideally use a stereoSolvePnP, but for now we triangulate
					vector<ofVec3f> checkerboardCornersWorldAll;
					{
						auto cameraANode = stereoCalibrateNode->getInput<Item::Camera>("Camera A");
						auto cameraBNode = stereoCalibrateNode->getInput<Item::Camera>("Camera B");

						//check nodes
						{
							if (!cameraANode || !cameraANode->getGrabber()) {
								throw(ofxRulr::Exception("Camera A is not available."));
							}
							if (!cameraBNode || !cameraBNode->getGrabber()) {
								throw(ofxRulr::Exception("Camera B is not available."));
							}
						}

						//get grabbers
						auto grabberA = cameraANode->getGrabber();
						auto grabberB = cameraBNode->getGrabber();

						//perform captures in parallel
						shared_ptr<ofxMachineVision::Frame> frameA;
						shared_ptr<ofxMachineVision::Frame> frameB;
						{
							auto futureCaptureA = std::async(std::launch::async, [&frameA, grabberA]() {
								return grabberA->getFreshFrame();
							});
							auto futureCaptureB = std::async(std::launch::async, [&frameB, grabberB]() {
								return grabberB->getFreshFrame();
							});

							frameA = futureCaptureA.get();
							frameB = futureCaptureB.get();

							if (!frameA) {
								throw(ofxRulr::Exception("Couldn't capture frame in camera A"));
							}
							if (!frameB) {
								throw(ofxRulr::Exception("Couldn't capture frame in camera B"));
							}
						}

						//perform board finds
						vector<cv::Point2f> imagePointsCameraA;
						vector<cv::Point2f> imagePointsCameraB;
						{
							auto findBoardMode = this->parameters.capture.stereoFindBoardMode.get();

							if (this->parameters.capture.stereoFindBoardMode.get() == Item::Board::FindBoardMode::Assistant) {
								//if we're using assistant don't perform in parallel
								if (!boardNode->findBoard(ofxCv::toCv(frameA->getPixels())
									, imagePointsCameraA
									, findBoardMode)) {
									throw(ofxRulr::Exception("Couldn't find board in camera A"));
								}

								if (!boardNode->findBoard(ofxCv::toCv(frameB->getPixels())
									, imagePointsCameraB
									, findBoardMode)) {
									throw(ofxRulr::Exception("Couldn't find board in camera B"));
								}
							} else
							{
								//perform the finds in parallel
								auto futureA = std::async(std::launch::async, [&frameA, &boardNode, &imagePointsCameraA, findBoardMode]() {
									if (!boardNode->findBoard(ofxCv::toCv(frameA->getPixels())
										, imagePointsCameraA
										, findBoardMode)) {
										throw(ofxRulr::Exception("Couldn't find board in camera A"));
									}
								});
								auto futureB = std::async(std::launch::async, [&frameB, &boardNode, &imagePointsCameraB, findBoardMode]() {
									if (!boardNode->findBoard(ofxCv::toCv(frameB->getPixels())
										, imagePointsCameraB
										, findBoardMode)) {
										throw(ofxRulr::Exception("Couldn't find board in camera B"));
									}
								});

								futureA.wait();
								futureB.wait();
							}
						}

						checkerboardCornersWorldAll = stereoCalibrateNode->triangulate(ofxCv::toOf(imagePointsCameraA)
							, ofxCv::toOf(imagePointsCameraB)
							, this->parameters.capture.correctStereoMatches);
					}

					//run the graycode scan with helper camera
					//(we do this second so that we can fail fast first)
					{
						graycodeNode->runScan();
						if (!graycodeNode->hasData()) {
							throw(ofxRulr::Exception("Graycode scan failed to produce a result"));
						}
					}

					//find checkerboard corners in projector image
					vector<ofVec3f> worldPoints;
					vector<cv::Point2f> projectorImagePoints;
					{
						auto & dataSet = graycodeNode->getDataSet();
						auto findBoardMode = this->parameters.capture.helperFindBoardMode.get();

						//copy out the median pixels (to avoid const issue)
						auto medianPixelsCopy = dataSet.getMedian();
						auto medianImage = ofxCv::toCv(medianPixelsCopy);

						//find the board in median
						vector<cv::Point2f> helperCameraImagePoints;
						if (!boardNode->findBoard(medianImage
							, helperCameraImagePoints
							, findBoardMode)) {
							throw(ofxRulr::Exception("Board not found in helper camera"));
						}

						//build the projectorImagePoints by searching and applying homography
						for (int i = 0; i < helperCameraImagePoints.size(); i++) {
							const auto & helperCameraImagePoint = helperCameraImagePoints[i];

							auto distanceThresholdSquared = this->parameters.capture.helperPixelsSeachDistance.get();
							distanceThresholdSquared *= distanceThresholdSquared;

							vector<cv::Point2f> cameraSpace;
							vector<cv::Point2f> projectorSpace;

							//build up search area
							for (const auto & pixel : dataSet) {
								const auto distanceSquared = pixel.getCameraXY().squareDistance(ofxCv::toOf(helperCameraImagePoint));
								if (distanceSquared < distanceThresholdSquared) {
									cameraSpace.push_back(ofxCv::toCv(pixel.getCameraXY()));
									projectorSpace.push_back(ofxCv::toCv(pixel.getCameraXY()));
								}
							}

							//if we didn't find anything
							if (cameraSpace.empty()) {
								//ignore this checkerboard corner
								continue;
							}

							auto homographyMatrix = ofxCv::findHomography(cameraSpace
								, projectorSpace
								, cv::Mat()
								, CV_RANSAC
								, 1);

							vector<cv::Point2f> cameraSpacePointsForHomography(1, helperCameraImagePoint);
							vector<cv::Point2f> projectionSpacePointsFromHomography(1);

							cv::perspectiveTransform(cameraSpacePointsForHomography
								, projectionSpacePointsFromHomography
								, homographyMatrix);

							worldPoints.push_back(checkerboardCornersWorldAll[i]);
							projectorImagePoints.push_back(projectionSpacePointsFromHomography[0]);
						}
					}

					if (worldPoints.empty()) {
						throw(ofxRulr::Exception("Could not resolve any corner positions in projector coordinates"));
					}

					auto capture = make_shared<Capture>();
					capture->worldSpacePoints = worldPoints;
					capture->imageSpacePoints = ofxCv::toOf(projectorImagePoints);
				}

				//----------
				void ProjectorFromStereoAndHelperCamera::calibrate() {

				}

				//----------
				void ProjectorFromStereoAndHelperCamera::drawWorld() {
					auto captures = this->captures.getSelection();
					for (const auto & capture : captures) {
						ofMesh line;
						line.setMode(ofPrimitiveMode::OF_PRIMITIVE_LINE_STRIP);
						for (auto worldSpacePoint : capture->worldSpacePoints) {
							line.addVertex(worldSpacePoint);
						}
						line.draw();
					}
				}

				//----------
				void ProjectorFromStereoAndHelperCamera::drawOnOutput(const ofRectangle & bounds) {
					auto captures = this->captures.getSelection();
					for (const auto & capture : captures) {
						ofMesh line;
						line.setMode(ofPrimitiveMode::OF_PRIMITIVE_LINE_STRIP);
						for (auto projectorPoint : capture->imageSpacePoints) {
							line.addVertex(projectorPoint);
						}
						line.draw();
					}
				}

				//----------
				void ProjectorFromStereoAndHelperCamera::serialize(Json::Value & json) {
					this->captures.serialize(json);
					Utils::Serializable::serialize(json, this->parameters);
				}

				//----------
				void ProjectorFromStereoAndHelperCamera::deserialize(const Json::Value & json) {
					this->captures.deserialize(json);
					Utils::Serializable::deserialize(json, this->parameters);
				}

				//----------
				void ProjectorFromStereoAndHelperCamera::populateInspector(ofxCvGui::InspectArguments & inspectArgs) {
					auto inspector = inspectArgs.inspector;
					inspector->addButton("Add capture", [this]() {
						try {
							Utils::ScopedProcess scopedProcess("Add capture");
							this->addCapture();
						}
						RULR_CATCH_ALL_TO_ALERT;
					}, ' ');
					inspector->addButton("Calibrate", [this]() {
						try {
							Utils::ScopedProcess scopedProcess("Calibrate");
							this->calibrate();
						}
						RULR_CATCH_ALL_TO_ALERT;
					}, OF_KEY_RETURN)->setHeight(100.0f);;
				}
			}
		}
	}
}
#include "pch_Plugin_Calibration.h"
#include "ProjectorFromStereoAndHelperCamera.h"

#include "StereoCalibrate.h"
#include "ofxRulr/Nodes/Procedure/Scan/Graycode.h"
#include "ofxRulr/Nodes/Item/Camera.h"
#include "ofxRulr/Nodes/Item/Projector.h"
#include "ofxRulr/Nodes/Item/AbstractBoard.h"
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
					json["reprojectedImageSpacePoints"] << this->reprojectedImageSpacePoints;
				}

				//----------
				void ProjectorFromStereoAndHelperCamera::Capture::deserialize(const Json::Value & json) {
					json["worldSpacePoints"] >> this->worldSpacePoints;
					json["imageSpacePoints"] >> this->imageSpacePoints;
					json["reprojectedImageSpacePoints"] >> this->reprojectedImageSpacePoints;
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
					RULR_NODE_DRAW_WORLD_LISTENER;

					this->addInput<Item::Projector>();
					this->addInput<StereoCalibrate>();
					this->addInput<Scan::Graycode>();
					this->addInput<Item::AbstractBoard>();

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
					auto boardNode = this->getInput<Item::AbstractBoard>();

					Utils::ThreadPool threadPool(2, 5);

					//find the checkerboard corners in stereo space
					// (this would ideally use a stereoSolvePnP, but for now we triangulate
					vector<ofVec3f> worldPointsFromStereo;
					vector<cv::Point3f> objectPointsFromStereo;
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
						vector<cv::Point2f> imagePointsA;
						vector<cv::Point2f> imagePointsB;
						{
							vector<cv::Point3f> objectPointsCameraA;
							vector<cv::Point3f> objectPointsCameraB;

							auto findBoardMode = this->parameters.capture.stereoFindBoardMode.get();

							if (this->parameters.capture.stereoFindBoardMode.get() == FindBoardMode::Assistant) {
								//if we're using assistant don't perform in parallel
								if (!boardNode->findBoard(ofxCv::toCv(frameA->getPixels())
									, imagePointsA
									, objectPointsCameraA
									, findBoardMode
									, cameraANode->getCameraMatrix()
									, cameraANode->getDistortionCoefficients())) {
									throw(ofxRulr::Exception("Couldn't find board in camera A"));
								}

								if (!boardNode->findBoard(ofxCv::toCv(frameB->getPixels())
									, imagePointsB
									, objectPointsCameraB
									, findBoardMode
									, cameraBNode->getCameraMatrix()
									, cameraBNode->getDistortionCoefficients())) {
									throw(ofxRulr::Exception("Couldn't find board in camera B"));
								}
							} else
							{
								//perform the finds in parallel
								auto futureA = std::async(std::launch::async, [&]() {
									if (!boardNode->findBoard(ofxCv::toCv(frameA->getPixels())
										, imagePointsA
										, objectPointsCameraA
										, findBoardMode
										, cameraANode->getCameraMatrix()
										, cameraANode->getDistortionCoefficients())) {
										throw(ofxRulr::Exception("Couldn't find board in camera A"));
									}
								});
								auto futureB = std::async(std::launch::async, [&]() {
									if (!boardNode->findBoard(ofxCv::toCv(frameB->getPixels())
										, imagePointsB
										, objectPointsCameraB
										, findBoardMode
										, cameraBNode->getCameraMatrix()
										, cameraBNode->getDistortionCoefficients())) {
										throw(ofxRulr::Exception("Couldn't find board in camera B"));
									}
								});

								futureA.wait();
								futureB.wait();
							}

							//find common board points
							Item::AbstractBoard::filterCommonPoints(imagePointsA
								, imagePointsB
								, objectPointsCameraA
								, objectPointsCameraB);							

							if (imagePointsA.size() == 0) {
								throw(ofxRulr::Exception("No common image points found between 2 cameras"));
							}

							objectPointsFromStereo = objectPointsCameraA;
						}

						worldPointsFromStereo = stereoCalibrateNode->triangulate(ofxCv::toOf(imagePointsA)
							, ofxCv::toOf(imagePointsB)
							, this->parameters.capture.correctStereoMatches);
					}

					//run the graycode scan with helper camera
					//(we do this second so that we can fail fast first)
					{
						if (!this->parameters.capture.useExistingGraycodeScan) {
							graycodeNode->runScan();
						}
						if (!graycodeNode->hasData()) {
							throw(ofxRulr::Exception("Graycode scan failed to produce a result"));
						}
					}

					//find board in projector image
					vector<ofVec3f> worldPoints;
					vector<cv::Point2f> projectorImagePoints;
					vector<cv::Point3f> objectPointsFromProjector;
					{
						Utils::ScopedProcess scopedProcessFindBoardInProjectorImage("Find Board in projector image", false);

						auto & dataSet = graycodeNode->getDataSet();
						auto findBoardMode = this->parameters.capture.helperFindBoardMode.get();

						//copy out the median pixels (to avoid const issue)
						auto medianPixelsCopy = dataSet.getMedian();
						auto medianImage = ofxCv::toCv(medianPixelsCopy);

						//find the board in median
						vector<cv::Point2f> helperCameraImagePoints;
						if (!boardNode->findBoard(medianImage
							, helperCameraImagePoints
							, objectPointsFromProjector
							, this->parameters.capture.helperFindBoardMode
							, cv::Mat()
							, cv::Mat())) {
							throw(ofxRulr::Exception("Board not found in helper camera"));
						}

						//check we have stuff in common with stereo
						Item::AbstractBoard::filterCommonPoints(worldPointsFromStereo
							, helperCameraImagePoints
							, objectPointsFromStereo
							, objectPointsFromProjector);

						{
							Utils::ScopedProcess scopedProcessFindBoardInProjectorImage("Find sub-pixel projector coordinates on board", false);
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
										projectorSpace.push_back(ofxCv::toCv(pixel.getProjectorXY()));
									}
								}

								//if we didn't find anything
								if (cameraSpace.empty()) {
									//ignore this checkerboard corner
									continue;
								}

								cv::Mat ransacMask;

								auto homographyMatrix = ofxCv::findHomography(cameraSpace
									, projectorSpace
									, ransacMask
									, CV_RANSAC
									, 1);

								if (homographyMatrix.empty()) {
									continue;
								}

								vector<cv::Point2f> cameraSpacePointsForHomography(1, helperCameraImagePoint);
								vector<cv::Point2f> projectionSpacePointsFromHomography(1);

								cv::perspectiveTransform(cameraSpacePointsForHomography
									, projectionSpacePointsFromHomography
									, homographyMatrix);

								worldPoints.push_back(worldPointsFromStereo[i]);
								projectorImagePoints.push_back(projectionSpacePointsFromHomography[0]);
							}
						}
					}

					if (worldPoints.empty()) {
						throw(ofxRulr::Exception("Could not resolve any corner positions in projector coordinates"));
					}

					auto capture = make_shared<Capture>();
					capture->worldSpacePoints = worldPoints;
					capture->imageSpacePoints = ofxCv::toOf(projectorImagePoints);
					this->captures.add(capture);
				}

				//----------
				void ProjectorFromStereoAndHelperCamera::calibrate() {
					this->throwIfMissingAConnection<Item::Projector>();
					this->throwIfMissingAConnection<System::VideoOutput>();

					auto videoOutputNode = this->getInput<System::VideoOutput>();
					if (!videoOutputNode->isWindowOpen()) {
						throw(ofxRulr::Exception("Window must be open to perform projector calibration"));
					}
					auto projectorNode = this->getInput<Item::Projector>();

					//set size of output
					projectorNode->setWidth(videoOutputNode->getWidth());
					projectorNode->setHeight(videoOutputNode->getHeight());

					vector<ofVec3f> worldPoints;
					vector<ofVec2f> projectorImagePoints;

					auto selectedCaptures = this->captures.getSelection();
					for (auto capture : selectedCaptures) {
						worldPoints.insert(worldPoints.end(), capture->worldSpacePoints.begin(), capture->worldSpacePoints.end());
						projectorImagePoints.insert(projectorImagePoints.end(), capture->imageSpacePoints.begin(), capture->imageSpacePoints.end());
					}

					if (worldPoints.size() != projectorImagePoints.size()) {
						throw(ofxRulr::Exception("Size mismatch error between world points and projector points"));
					}

					auto count = worldPoints.size();
					if (count < 6) { //unlikely it'd be between 0 and 100 anyway
						throw(ofxRulr::Exception("Not enough points to calibrate projector"));
					}

					cv::Mat cameraMatrix, distortionCoefficients, rotationMatrix, translation;
					auto size = projectorNode->getSize();

					int decimation = count / 16;
					if (!this->parameters.calibrate.useDecimation || decimation <= 1) {
						this->reprojectionError = ofxCv::calibrateProjector(cameraMatrix
							, rotationMatrix
							, translation
							, worldPoints
							, projectorImagePoints
							, size.width
							, size.height
							, false
							, this->parameters.calibrate.initialLensOffset
							, this->parameters.calibrate.initialThrowRatio);
					}
					else {
						Utils::ScopedProcess scopedProcessDecimatedFit("Decimated fit", false, log(decimation) / log(2) + 1);

						vector<cv::Point2f> remainingImagePoints(ofxCv::toCv(projectorImagePoints));
						vector<cv::Point3f> remainingWorldPoints(ofxCv::toCv(worldPoints));

						vector<cv::Point2f> imagePointsDecimated;
						vector<cv::Point3f> worldPointsDecimated;

						auto flags = CV_CALIB_FIX_K1 | CV_CALIB_FIX_K2 | CV_CALIB_FIX_K3 | CV_CALIB_FIX_K4 | CV_CALIB_FIX_K5 | CV_CALIB_FIX_K6
							| CV_CALIB_ZERO_TANGENT_DIST | CV_CALIB_USE_INTRINSIC_GUESS | CV_CALIB_FIX_ASPECT_RATIO;
						vector<cv::Mat> rotationVector, translationVector;

						decimation *= 2;
						do {
							decimation /= 2;

							int remainingCount = remainingImagePoints.size();
							auto remainingImagePointsIterator = remainingImagePoints.begin();
							auto remainingWorldPointsIterator = remainingWorldPoints.begin();

							for (int i = 0; i < remainingCount; i++) {
								if (i % (decimation + 1) == 0) {
									imagePointsDecimated.emplace_back(move(*remainingImagePointsIterator));
									worldPointsDecimated.emplace_back(move(*remainingWorldPointsIterator));

									remainingImagePointsIterator = remainingImagePoints.erase(remainingImagePointsIterator);
									remainingWorldPointsIterator = remainingWorldPoints.erase(remainingWorldPointsIterator);
								}
								else {
									remainingImagePointsIterator++;
									remainingWorldPointsIterator++;
								}
							}

							Utils::ScopedProcess scopedProcessDecimationInner("Decimation by " + ofToString(decimation) + " with " + ofToString(imagePointsDecimated.size()) + "points", false);

							//build a camera matrix + distortion coefficients if we don't have yet
							if (cameraMatrix.empty()) {
								cameraMatrix = cv::Mat::eye(3, 3, CV_64F);
								cameraMatrix.at<double>(0, 0) = size.width * this->parameters.calibrate.initialThrowRatio;
								cameraMatrix.at<double>(1, 1) = size.width * this->parameters.calibrate.initialThrowRatio;
								cameraMatrix.at<double>(0, 2) = size.width / 2.0f;
								cameraMatrix.at<double>(1, 2) = size.height * (0.50f - this->parameters.calibrate.initialLensOffset / 2.0f);
								distortionCoefficients = cv::Mat::zeros(5, 1, CV_64F);
							}
							else {
								//ensure positive focal length
								cameraMatrix.at<double>(0, 0) = ofClamp(cameraMatrix.at<double>(0, 0), 1, 100 * (float)size.width);
								cameraMatrix.at<double>(1, 1) = ofClamp(cameraMatrix.at<double>(1, 1), 1, 100 * (float)size.width);

								//calibrateCamera doesn't like if principal point is outside of image, so we clamp it
								cameraMatrix.at<double>(0, 2) = ofClamp(cameraMatrix.at<double>(0, 2), 0.01, 0.99 * (float)size.width);
								cameraMatrix.at<double>(1, 2) = ofClamp(cameraMatrix.at<double>(1, 2), 0.01, 0.99 * (float)size.height);
							}

							auto error = cv::calibrateCamera(vector<vector<cv::Point3f>>(1, worldPointsDecimated)
								, vector<vector<cv::Point2f>>(1, imagePointsDecimated)
								, size
								, cameraMatrix
								, distortionCoefficients
								, rotationVector
								, translationVector
								, flags);

							cout << "Reprojection error = " << error << "px" << endl;
							this->reprojectionError = error;
						} while (decimation > 0);

						rotationMatrix = rotationVector[0];
						translation = translationVector[0];
					}

					//remove outliers
					if (this->parameters.calibrate.removeOutliers.enabled) {
						vector<cv::Point2f> reprojectedPoints;
						cv::projectPoints(ofxCv::toCv(worldPoints)
							, rotationMatrix
							, translation
							, cameraMatrix
							, distortionCoefficients
							, reprojectedPoints);

						vector<cv::Point3f> worldPointsNoOutliers;
						vector<cv::Point2f> projectorImagePointsNoOutliers;

						auto thresholdSquared = pow(this->parameters.calibrate.removeOutliers.maxReprojectionError, 2);
						for (int i = 0; i < count; i++) {
							if (projectorImagePoints[i].squareDistance(ofxCv::toOf(reprojectedPoints[i])) < thresholdSquared) {
								worldPointsNoOutliers.emplace_back(ofxCv::toCv(worldPoints[i]));
								projectorImagePointsNoOutliers.emplace_back(ofxCv::toCv(projectorImagePoints[i]));
							}
						}

						cout << "fitting with " << worldPointsNoOutliers.size() << " points reduced" << endl;

						this->reprojectionError = ofxCv::calibrateProjector(cameraMatrix
							, rotationMatrix
							, translation
							, worldPoints
							, projectorImagePoints
							, size.width
							, size.height
							, false
							, this->parameters.calibrate.initialLensOffset
							, this->parameters.calibrate.initialThrowRatio);
					}

					//reproject points for preview
					{
						for (auto capture : selectedCaptures) {
							cv::projectPoints(ofxCv::toCv(capture->worldSpacePoints)
								, rotationMatrix
								, translation
								, cameraMatrix
								, distortionCoefficients
								, ofxCv::toCv(capture->reprojectedImageSpacePoints));
						}
					}

					projectorNode->setExtrinsics(rotationMatrix, translation, true);
					projectorNode->setIntrinsics(cameraMatrix);
				}

				//----------
				void ProjectorFromStereoAndHelperCamera::drawWorldStage() {
					if (this->parameters.draw.dataInWorld) {
						auto captures = this->captures.getSelection();
						for (const auto & capture : captures) {
							ofPushStyle();
							{
								ofSetColor(capture->color);
								ofMesh line;
								line.setMode(ofPrimitiveMode::OF_PRIMITIVE_LINE_STRIP);
								for (auto worldSpacePoint : capture->worldSpacePoints) {
									line.addVertex(worldSpacePoint);
								}
								line.draw();
							}
							ofPopStyle();
						}
					}
				}

				//----------
				void ProjectorFromStereoAndHelperCamera::drawOnOutput(const ofRectangle & bounds) {
					auto captures = this->captures.getSelection();
					for (const auto & capture : captures) {
						//draw data points as lines
						if (this->parameters.draw.dataOnVideoOutput) {
							ofPushStyle();
							{
								ofSetColor(capture->color);
								ofMesh line;
								line.setMode(ofPrimitiveMode::OF_PRIMITIVE_LINE_STRIP);
								for (auto projectorPoint : capture->imageSpacePoints) {
									line.addVertex(projectorPoint);
								}
								line.draw();
							}
							ofPopStyle();
						}

						//draw reprojected points
						if (this->parameters.draw.reprojectedOnVideoOutput) {
							ofPushStyle();
							{
								ofSetColor(255, 0, 0);
								for (const auto reprojectedPoint : capture->reprojectedImageSpacePoints) {
									ofPushMatrix();
									{
										ofTranslate(reprojectedPoint);
										ofDrawLine(-5, -5, +5, +5);
										ofDrawLine(-5, +5, +5, -5);
									}
									ofPopMatrix();
								}
							}
							ofPopStyle();
						}

					}
				}

				//----------
				void ProjectorFromStereoAndHelperCamera::serialize(Json::Value & json) {
					this->captures.serialize(json);
					Utils::Serializable::serialize(json, this->parameters);
					Utils::Serializable::serialize(json, this->reprojectionError);
				}

				//----------
				void ProjectorFromStereoAndHelperCamera::deserialize(const Json::Value & json) {
					this->captures.deserialize(json);
					Utils::Serializable::deserialize(json, this->parameters);
					Utils::Serializable::deserialize(json, this->reprojectionError);
				}

				//----------
				void ProjectorFromStereoAndHelperCamera::populateInspector(ofxCvGui::InspectArguments & inspectArgs) {
					auto inspector = inspectArgs.inspector;
					inspector->addButton("Add capture", [this]() {
						try {
							Utils::ScopedProcess scopedProcess("Add capture");
							this->addCapture();
							scopedProcess.end();
						}
						RULR_CATCH_ALL_TO_ALERT;
					}, ' ');
					
					this->captures.populateWidgets(inspector);

					inspector->addButton("Calibrate", [this]() {
						try {
							Utils::ScopedProcess scopedProcess("Calibrate");
							this->calibrate();
							scopedProcess.end();
						}
						RULR_CATCH_ALL_TO_ALERT;
					}, OF_KEY_RETURN)->setHeight(100.0f);

					inspector->addLiveValue<float>(this->reprojectionError);
				}
			}
		}
	}
}
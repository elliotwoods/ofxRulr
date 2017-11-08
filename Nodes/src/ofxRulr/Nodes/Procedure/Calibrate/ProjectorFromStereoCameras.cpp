#include "pch_RulrNodes.h"
#include "ProjectorFromStereoCameras.h"

#include "StereoCalibrate.h"
#include "ofxRulr/Nodes/Item/Camera.h"
#include "ofxRulr/Nodes/Item/Projector.h"
#include "ofxRulr/Nodes/System/VideoOutput.h"

#include "ofxGraycode.h"

namespace ofxRulr {
	namespace Nodes {
		namespace Procedure {
			namespace Calibrate {
#pragma mark Capture
				//----------
				ProjectorFromStereoCamera::Capture::Capture() {
					RULR_SERIALIZE_LISTENERS;
				}

				//----------
				std::string ProjectorFromStereoCamera::Capture::getDisplayString() const {
					stringstream ss;
					ss << "Points : " << endl << this->dataPoints.getNumVertices();
					return ss.str();
				}

				//----------
				void ProjectorFromStereoCamera::Capture::serialize(Json::Value & json) {
					json["dataPoints"] << this->dataPoints;
				}

				//----------
				void ProjectorFromStereoCamera::Capture::deserialize(const Json::Value & json) {
					json["dataPoints"] >> this->dataPoints;
				}

				//----------
				void ProjectorFromStereoCamera::Capture::drawWorld() {
					this->dataPoints.drawVertices();
				}

#pragma mark ProjectorFromStereoCamera
				//----------
				ProjectorFromStereoCamera::ProjectorFromStereoCamera() {
					RULR_NODE_INIT_LISTENER;
				}

				//----------
				std::string ProjectorFromStereoCamera::getTypeName() const {
					return "Procedure::Calibrate::ProjectorFromStereoCamera";
				}

				//----------
				void ProjectorFromStereoCamera::init() {
					RULR_NODE_UPDATE_LISTENER;
					RULR_NODE_DRAW_WORLD_LISTENER;
					RULR_NODE_SERIALIZATION_LISTENERS;
					RULR_NODE_INSPECTOR_LISTENER;

					this->addInput<StereoCalibrate>();
					this->addInput<Item::Projector>();
					this->addInput<System::VideoOutput>();

					{
						auto panel = ofxCvGui::Panels::makeWidgets();
						this->captures.populateWidgets(panel);
						this->panel = panel;
					}
				}

				//----------
				void ProjectorFromStereoCamera::update() {

				}

				//----------
				void ProjectorFromStereoCamera::drawWorld() {
					auto & shader = ofxAssets::shader("ofxRulr::Nodes::Procedure::Calibrate::coloredPointCloud");

					shader.begin();
					{
						shader.setUniform1f("brightnessBoost", this->parameters.preview.brightnessBoost);

						auto selectedCaptures = this->captures.getSelection();
						for (auto & capture : selectedCaptures) {
							shader.setUniform4f("color", (ofFloatColor) capture->color.get());
							capture->drawWorld();
						}
					}
					shader.end();
				}

				//----------
				void ProjectorFromStereoCamera::serialize(Json::Value & json) {
					this->captures.serialize(json);
					Utils::Serializable::serialize(json, this->parameters);
					Utils::Serializable::serialize(json, this->reprojectionError);
				}

				//----------
				void ProjectorFromStereoCamera::deserialize(const Json::Value & json) {
					this->captures.deserialize(json);
					Utils::Serializable::deserialize(json, this->parameters);
					Utils::Serializable::deserialize(json, this->reprojectionError);
				}

				//----------
				void ProjectorFromStereoCamera::populateInspector(ofxCvGui::InspectArguments & inspectArgs) {
					auto inspector = inspectArgs.inspector;

					inspector->addButton("Add capture", [this]() {
						try {
							Utils::ScopedProcess scopedProcess("Add capture");
							this->addCapture();
							scopedProcess.end();
						}
						RULR_CATCH_ALL_TO_ALERT;
						ofShowCursor(); // just in case we throw
					}, ' ');

					inspector->addButton("Calibrate", [this]() {
						try {
							Utils::ScopedProcess scopedProcess("Calibrating");
							this->calibrate();
							scopedProcess.end();
						}
						RULR_CATCH_ALL_TO_ALERT;
					}, OF_KEY_RETURN)->setHeight(100.0f);

					inspector->addLiveValue<float>(this->reprojectionError);

					inspector->addParameterGroup(this->parameters);
				}

				//----------
				ofxCvGui::PanelPtr ProjectorFromStereoCamera::getPanel() {
					return this->panel;
				}

				//----------
				void ProjectorFromStereoCamera::addCapture() {
					this->throwIfMissingAnyConnection();

					auto stereoCalibrateNode = this->getInput<StereoCalibrate>();
					auto cameraANode = stereoCalibrateNode->getInput<Item::Camera>("Camera A");
					auto cameraBNode = stereoCalibrateNode->getInput<Item::Camera>("Camera B");
					auto videoOutputNode = this->getInput<System::VideoOutput>();
					auto projectorNode = this->getInput<Item::Projector>();

					if (!videoOutputNode->isWindowOpen()) {
						throw(ofxRulr::Exception("Window is not open"));
					}
					if (!cameraANode || !cameraANode->getGrabber()) {
						throw(ofxRulr::Exception("Camera A is not available."));
					}
					if (!cameraBNode || !cameraBNode->getGrabber()) {
						throw(ofxRulr::Exception("Camera B is not available."));
					}
					auto grabberA = cameraANode->getGrabber();
					auto grabberB = cameraBNode->getGrabber();

					auto payload = make_unique<ofxGraycode::PayloadGraycode>();
					payload->init(videoOutputNode->getWidth(), videoOutputNode->getHeight());

					auto encoder = make_unique<ofxGraycode::Encoder>();
					auto decoderA = make_unique<ofxGraycode::Decoder>();
					auto decoderB = make_unique<ofxGraycode::Decoder>();
					encoder->init(*payload);
					decoderA->init(*payload);
					decoderB->init(*payload);

					decoderA->setThreshold(this->parameters.scan.threshold);
					decoderB->setThreshold(this->parameters.scan.threshold);

					//set the projector resolution
					projectorNode->setWidth(payload->getWidth());
					projectorNode->setHeight(payload->getHeight());

					//from Graycode.cpp
					{
						ofHideCursor();
						Utils::ScopedProcess scopedProcess("Scanning graycode", true, payload->getFrameCount());
						ofImage message;
						while (*encoder >> message) {
							Utils::ScopedProcess frameScopedProcess("Scanning frame", false);

							//present the frame
							videoOutputNode->clearFbo(false);
							videoOutputNode->begin();
							{
								ofPushStyle();
								{
									auto brightness = this->parameters.scan.brightness;
									ofSetColor(brightness);
									message.draw(0, 0);
								}
								ofPopStyle();
							}
							videoOutputNode->end();
							videoOutputNode->presentFbo();

							//delay between present and capture
							auto startWait = ofGetElapsedTimeMillis();
							while (ofGetElapsedTimeMillis() - startWait < this->parameters.scan.captureDelay) {
								ofSleepMillis(1);
								grabberA->update();
								grabberB->update();
							}

							//get the frames
							{
								auto frame = grabberA->getFreshFrame();
								if (!frame) {
									throw(ofxRulr::Exception("Couldn't get fresh frame from camera"));
								}
								*decoderA << frame->getPixels();
							}
							{
								auto frame = grabberB->getFreshFrame();
								if (!frame) {
									throw(ofxRulr::Exception("Couldn't get fresh frame from camera"));
								}
								*decoderB << frame->getPixels();
							}
						}
						scopedProcess.end();
					}

					{
						Utils::ScopedProcess scopedProcessTriangulate("Triangulating points");

						struct GraycodePixel {
							ofVec2f cameraImageSpace;
							ofVec2f projectorImageSpace;
							float brightness;
							uint32_t distance;
						};

						struct DataPoint {
							ofVec2f projectorImageSpace;
							ofVec2f cameraImageSpaceA;
							ofVec2f cameraImageSpaceB;
							float color;
						};

						vector<DataPoint> dataPoints;
						//Find image point pairs
						{
							map<int, GraycodePixel> projectorPixelsA;
							for (const auto & pixel : decoderA->getDataSet()) {
								if (pixel.active) {
									auto graycodePixel = GraycodePixel{
										pixel.getCameraXY(),
										pixel.getProjectorXY(),
										(float)pixel.median,
										pixel.distance
									};

									//check if pixel already exists
									auto findPixel = projectorPixelsA.find(pixel.projector);
									if (findPixel == projectorPixelsA.end()) {
										//first time to see
										projectorPixelsA.emplace(pixel.projector, graycodePixel);
									}
									else {
										//we've seen this projector pixel before
										if (pixel.distance > findPixel->second.distance) {
											//update existing if our distance is larger
											findPixel->second = graycodePixel;
										}
									}
								}
							}

							map<int, GraycodePixel> projectorPixelsB;
							for (const auto pixel : decoderB->getDataSet()) {
								if (pixel.active) {
									projectorPixelsB.emplace(pixel.projector, GraycodePixel{
										pixel.getCameraXY(),
										pixel.getProjectorXY(),
										(float) pixel.median
									});
								}
							}

							//add any which are in both
							for (auto & projectorPixelA : projectorPixelsA) {
								auto projectorPixelB = projectorPixelsB.find(projectorPixelA.first);
								if (projectorPixelB != projectorPixelsB.end()) {
									//this projector pixel is seen in both cameras
									dataPoints.push_back(DataPoint{
										projectorPixelA.second.projectorImageSpace,
										projectorPixelA.second.cameraImageSpace,
										projectorPixelB->second.cameraImageSpace,
										max(projectorPixelA.second.brightness, projectorPixelB->second.brightness) / 255.0f
									});
								}
							}
						}

						//triangulate points
						{
							vector<ofVec2f> imagePointsA;
							vector<ofVec2f> imagePointsB;
							vector<ofVec2f> projectionSpacePoints;
							vector<ofFloatColor> colors;

							for (const auto & dataPoint : dataPoints) {
								imagePointsA.push_back(dataPoint.cameraImageSpaceA);
								imagePointsB.push_back(dataPoint.cameraImageSpaceB);
								projectionSpacePoints.push_back(dataPoint.projectorImageSpace);
								colors.push_back(dataPoint.color);
							}

							auto worldPoints = stereoCalibrateNode->triangulate(imagePointsA, imagePointsB, this->parameters.scan.correctStereoMatches);

							auto capture = make_shared<Capture>();
							
							capture->dataPoints.clear();
							capture->dataPoints.getVertices() = worldPoints;
							capture->dataPoints.getColors() = colors;
							capture->dataPoints.getTexCoords() = projectionSpacePoints;
							this->captures.add(capture);
						}

						scopedProcessTriangulate.end();
					}
				}

				//----------
				void ProjectorFromStereoCamera::calibrate() {
					this->throwIfMissingAConnection<Item::Projector>();

					auto projectorNode = this->getInput<Item::Projector>();
					vector<ofVec3f> worldPoints;
					vector<ofVec2f> projectorImagePoints;

					auto selectedCaptures = this->captures.getSelection();
					for (auto capture : selectedCaptures) {
						auto & captureWorldPoints = capture->dataPoints.getVertices();
						worldPoints.insert(worldPoints.end(), captureWorldPoints.begin(), captureWorldPoints.end());

						auto & captureProjectorPoints = capture->dataPoints.getTexCoords();
						projectorImagePoints.insert(projectorImagePoints.end(), captureProjectorPoints.begin(), captureProjectorPoints.end());
					}

					if (worldPoints.size() != projectorImagePoints.size()) {
						throw(ofxRulr::Exception("Size mismatch error between world points and projector points"));
					}

					auto count = worldPoints.size();
					if (count < 6) { //unlikely it'd be between 0 and 100 anyway
						throw(ofxRulr::Exception("Not enough points to calibrate projector"));
					}

					cv::Mat cameraMatrix, distortionCoefficients, rotation, translation;
					auto size = projectorNode->getSize();


					auto decimation = count / 16;
					if (!this->parameters.calibrate.useDecimation || decimation <= 1) {
						this->reprojectionError = ofxCv::calibrateProjector(cameraMatrix
							, rotation
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

						rotation = rotationVector[0];
						translation = translationVector[0];
					}

					//remove outliers
					if (this->parameters.calibrate.removeOutliers.enabled) {
						vector<cv::Point2f> reprojectedPoints;
						cv::projectPoints(ofxCv::toCv(worldPoints)
							, rotation
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
							, rotation
							, translation
							, worldPoints
							, projectorImagePoints
							, size.width
							, size.height
							, false
							, this->parameters.calibrate.initialLensOffset
							, this->parameters.calibrate.initialThrowRatio);
					}

					projectorNode->setExtrinsics(rotation, translation, true);
					projectorNode->setIntrinsics(cameraMatrix);
				}
			}
		}
	}
}
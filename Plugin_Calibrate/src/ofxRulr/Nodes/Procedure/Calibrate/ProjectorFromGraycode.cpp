#include "pch_Plugin_Calibrate.h"
#include "ProjectorFromGraycode.h"

#include "ofxRulr/Nodes/Procedure/Scan/Graycode.h"
#include "ofxRulr/Nodes/Item/Camera.h"
#include "ofxRulr/Nodes/Item/Projector.h"
#include "ofxRulr/Nodes/Item/AbstractBoard.h"

#include "ofxTriangle.h"

using namespace ofxCv;

namespace ofxRulr {
	namespace Nodes {
		namespace Procedure {
			namespace Calibrate {
#pragma mark Capture
				//----------
				ProjectorFromGraycode::Capture::Capture() {
					RULR_NODE_SERIALIZATION_LISTENERS;
				}

				//----------
				string ProjectorFromGraycode::Capture::getDisplayString() const {
					stringstream ss;
					ss << this->worldPoints.size() << " points";
					return ss.str();
				}

				//----------
				void ProjectorFromGraycode::Capture::serialize(nlohmann::json & json) {
					json["worldPoints"] << this->worldPoints;
					json["projectorImagePoints"] << this->projectorImagePoints;
				}

				//----------
				void ProjectorFromGraycode::Capture::deserialize(const nlohmann::json & json) {
					json["worldPoints"] >> this->worldPoints;
					json["projectorImagePoints"] >> this->projectorImagePoints;
				}

				//----------
				void ProjectorFromGraycode::Capture::drawWorld(shared_ptr<Item::Projector> projector)
				{
					ofMesh points;
					points.setMode(ofPrimitiveMode::OF_PRIMITIVE_POINTS);

					ofMesh zigzag;
					zigzag.setMode(ofPrimitiveMode::OF_PRIMITIVE_LINE_STRIP);

					for (int i = 0; i < this->worldPoints.size(); i++) {
						const auto & worldPoint = this->worldPoints[i];
						if (i == 0) {
							ofPushStyle();
							{
								ofSetColor(this->color);
								ofDrawSphere(worldPoint, 0.05f);
							}
							ofPopStyle();
						}

						zigzag.addVertex(worldPoint);
						zigzag.addColor((float)i / (float)this->worldPoints.size());
					}
					zigzag.draw();

					Utils::Graphics::pushPointSize(5.0f);
					{
						points.drawVertices();
					}
					Utils::Graphics::popPointSize();

					//draw transform axes
					ofPushMatrix();
					{
						ofMultMatrix(this->transform);
						ofDrawAxis(0.1f);
					}
					ofPopMatrix();

					//draw lines to projector
					if (projector) {
						const auto projectorWidth = projector->getWidth();
						const auto projectorHeight = projector->getHeight();
						const auto projectorPosition = projector->getPosition();
						const auto projectorView = projector->getViewInWorldSpace();

						ofMesh projectorRays;
						projectorRays.setMode(ofPrimitiveMode::OF_PRIMITIVE_TRIANGLES);

						for (int i = 0; i < this->worldPoints.size(); i++) {
							const auto & worldPoint = this->worldPoints[i];
							auto color = ofFloatColor(
								this->projectorImagePoints[i].x / projector->getWidth(),
								this->projectorImagePoints[i].y / projector->getHeight(),
								0
							);
							points.addVertex(worldPoint);
							points.addColor(color);

							auto distance = glm::distance(worldPoint, projectorPosition);
							auto projectorRay = projectorView.castPixel(this->projectorImagePoints[i], false);
							projectorRays.addVertex(projectorRay.s);
							projectorRays.addVertex(projectorRay.s + projectorRay.t * distance / glm::length(projectorRay.t));
							projectorRays.addVertex(worldPoint);

							projectorRays.addColor(ofFloatColor(0.5, 0.5, 0.5, 0.0f));
							color.a = 0.5f;
							projectorRays.addColor(color);
							color.a = 1.0f;
							projectorRays.addColor(color);
						}

						ofPushStyle();
						{
							ofEnableAlphaBlending();
							projectorRays.drawWireframe();
							ofDisableAlphaBlending();
						}
						ofPopStyle();
					}
					else {
						for (int i = 0; i < this->worldPoints.size(); i++) {
							const auto & worldPoint = this->worldPoints[i];
							points.addVertex(worldPoint);
						}
					}
				}

				//----------
				void ProjectorFromGraycode::Capture::drawOnCameraImage() {
					ofPushStyle();
					{
						ofSetColor(this->color);
						ofxCv::drawCorners(ofxCv::toCv(this->cameraImagePoints), false);
					}
					ofPopStyle();
				}

				//----------
				void ProjectorFromGraycode::Capture::drawOnProjectorImage() {
					ofPushStyle();
					{
						ofSetColor(this->color);
						ofxCv::drawCorners(ofxCv::toCv(this->projectorImagePoints), false);
					}
					ofPopStyle();
				}

#pragma mark ProjectorFromGraycode
				//----------
				ProjectorFromGraycode::ProjectorFromGraycode() {
					RULR_NODE_INIT_LISTENER;
				}

				//----------
				string ProjectorFromGraycode::getTypeName() const {
					return "Procedure::Calibrate::ProjectorFromGraycode";
				}

				//----------
				ofxCvGui::PanelPtr ProjectorFromGraycode::getPanel() {
					return this->panel;
				}

				//----------
				void ProjectorFromGraycode::init() {
					RULR_NODE_UPDATE_LISTENER;
					RULR_NODE_DRAW_WORLD_LISTENER;
					RULR_NODE_INSPECTOR_LISTENER;
					RULR_NODE_SERIALIZATION_LISTENERS;

					this->addInput<Item::Projector>();
					this->addInput<Item::Camera>();
					this->addInput<Item::AbstractBoard>();
					this->addInput<Scan::Graycode>();

					//for drawing on output
					auto videoOutputPin = this->addInput<System::VideoOutput>();
					videoOutputListener = make_unique<Utils::VideoOutputListener>(videoOutputPin, [this](const ofRectangle & bounds) {
						this->drawOnVideoOutput(bounds);
					});

					{
						auto panel = ofxCvGui::Panels::Groups::makeStrip();
						
						{
							auto cameraPanel = ofxCvGui::Panels::makeTexture(this->preview.projectorInCamera, "Camera");
							cameraPanel->onDrawImage += [this](ofxCvGui::DrawImageArguments &) {
								auto captures = this->captures.getSelection();
								for (auto capture : captures) {
									capture->drawOnCameraImage();
								}
							};
							panel->add(cameraPanel);
						}
						{
							auto projectorPanel = ofxCvGui::Panels::makeTexture(this->preview.cameraInProjector, "Projector");
							projectorPanel->onDrawImage += [this](ofxCvGui::DrawImageArguments &) {
								auto captures = this->captures.getSelection();
								for (auto capture : captures) {
									capture->drawOnProjectorImage();
								}
							};
							panel->add(projectorPanel);
						}
						this->panel = panel;
					}
				}

				//----------
				void ProjectorFromGraycode::update() {
					{
						auto projector = this->getInput<Item::Projector>();
						if (projector) {
							auto width = this->preview.cameraInProjector.getWidth();
							auto height = this->preview.cameraInProjector.getHeight();
							if (width != projector->getWidth()
								|| height != projector->getHeight()) {
// 								this->preview.cameraInProjector.allocate(projector->getWidth()
// 									, projector->getHeight()
// 									, GL_LUMINANCE);
							}
						}
					}
					{
						auto camera = this->getInput<Item::Camera>();
						if (camera) {
							auto width = this->preview.projectorInCamera.getWidth();
							auto height = this->preview.projectorInCamera.getHeight();
							if (width != camera->getWidth()
								|| height != camera->getHeight()) {
// 								this->preview.projectorInCamera.allocate(camera->getWidth()
// 									, camera->getHeight()
// 									, GL_LUMINANCE);
							}
						}
					}
				}

				//----------
				void ProjectorFromGraycode::drawWorldStage() {
					auto projectorNode = this->getInput<Item::Projector>();

					float projectorWidth = 0, projectorHeight = 0;
					if (projectorNode) {
						projectorWidth = projectorNode->getWidth();
						projectorHeight = projectorNode->getHeight();
					}

					auto captures = this->captures.getSelection();
					for (auto & capture : captures) {
						capture->drawWorld(projectorNode);
					}
				}

				//----------
				//from http://stackoverflow.com/questions/13299409/how-to-get-the-image-pixel-at-real-locations-in-opencv#
				template<typename Type> // Type could be Vec3b for RGB8
				Type getColorSubpix(const cv::Mat& img, cv::Point2f pt)
				{
					cv::Mat patch;
					cv::remap(img, patch, cv::Mat(1, 1, CV_32FC2, &pt), cv::noArray(),
						cv::INTER_LINEAR, cv::BORDER_REFLECT_101);
					return patch.at<Type>(0, 0);
				}

				//----------
				void ProjectorFromGraycode::addCapture() {
					Utils::ScopedProcess scopedProcess("Add capture");

					this->throwIfMissingAConnection<Procedure::Scan::Graycode>();
					this->throwIfMissingAConnection<Item::Camera>();
					this->throwIfMissingAConnection<Item::AbstractBoard>();

					auto graycodeNode = this->getInput<Procedure::Scan::Graycode>();
					auto cameraNode = this->getInput<Item::Camera>();
					auto boardNode = this->getInput<Item::AbstractBoard>();
					
					//Graycode scan
					if (!this->parameters.capture.useExistingGraycodeScan) {
						Utils::ScopedProcess scopedProcess("Graycode scan", false);
						graycodeNode->runScan();
					}

					auto & dataSet = graycodeNode->getDataSet();
					if (!dataSet.getHasData()) {
						throw(ofxRulr::Exception("Graycode node has no data"));
					}

					//Make previews (use graycode data)
					{
						this->preview.projectorInCamera.loadData(dataSet.getMedian());
						this->preview.cameraInProjector.loadData(dataSet.getMedianInverse());
					}

					//Find board in camera
					vector<cv::Point2f> cameraImagePoints;
					vector<cv::Point3f> boardObjectPoints;
					{
						Utils::ScopedProcess scopedProcessFindBoard("Find board in camera image", false);
						const auto & median = dataSet.getMedian();

						ofPixels medianCopy(median);
						auto medianCopyMat = toCv(medianCopy);

						if (!boardNode->findBoard(medianCopyMat
							, cameraImagePoints
							, boardObjectPoints
							, this->parameters.capture.findBoardMode
							, cameraNode->getCameraMatrix()
							, cameraNode->getDistortionCoefficients())) {
							throw(ofxRulr::Exception("Board not found in camera image"));
						}

						if (cameraImagePoints.size() < 4) {
							throw(ofxRulr::Exception("Found less than 4 image points (" + ofToString(cameraImagePoints.size()) + ")"));
						}
					}
					
					//find pose of board in 3D space
					glm::mat4 boardTransform;
					{
						Mat rotation, translation;

						cv::solvePnP(boardObjectPoints
							, cameraImagePoints
							, cameraNode->getCameraMatrix()
							, cameraNode->getDistortionCoefficients()
							, rotation
							, translation);
						
						boardTransform = makeMatrix(rotation, translation);
					}
					
					//find 3d points
					vector<glm::vec3> boardPointsInWorldSpace;
					{
						for (auto & boardObjectPoint : boardObjectPoints) {
							boardPointsInWorldSpace.push_back(Utils::applyTransform(boardTransform * cameraNode->getTransform(), toOf(boardObjectPoint)));
						}
					}

					auto capture = make_shared<Capture>();
					{
						Utils::ScopedProcess scopedProcessFindBoardInProjectorImage("Find sub-pixel projector coordinates on board", false);
						//build the projectorImagePoints by searching and applying homography
						for (int i = 0; i < cameraImagePoints.size(); i++) {
							const auto & cameraImagePoint = cameraImagePoints[i];

							auto distanceThresholdSquared = this->parameters.capture.pixelSearchDistance.get();
							distanceThresholdSquared *= distanceThresholdSquared;

							vector<cv::Point2f> cameraSpace;
							vector<cv::Point2f> projectorSpace;

							//build up search area
							for (const auto & pixel : dataSet) {
								const auto distanceSquared = glm::length2(pixel.getCameraXY() - ofxCv::toOf(cameraImagePoint));
								if (distanceSquared < distanceThresholdSquared) {
									cameraSpace.push_back(ofxCv::toCv(pixel.getCameraXY()));
									projectorSpace.push_back(ofxCv::toCv(pixel.getProjectorXY()));
								}
							}

							//if we didn't find enough data
							if (cameraSpace.size() < 6) {
								//ignore this checkerboard corner
								continue;
							}

							cv::Mat ransacMask;

							auto homographyMatrix = ofxCv::findHomography(cameraSpace
								, projectorSpace
								, ransacMask
								, cv::RANSAC
								, 1);

							if (homographyMatrix.empty()) {
								continue;
							}

							vector<cv::Point2f> cameraSpacePointsForHomography(1, cameraImagePoint);
							vector<cv::Point2f> projectionSpacePointsFromHomography(1);

							cv::perspectiveTransform(cameraSpacePointsForHomography
								, projectionSpacePointsFromHomography
								, homographyMatrix);

							capture->worldPoints.push_back(boardPointsInWorldSpace[i]);
							capture->cameraImagePoints.push_back(ofxCv::toOf(cameraImagePoints[i]));
							capture->projectorImagePoints.push_back(ofxCv::toOf(projectionSpacePointsFromHomography[0]));
						}
						this->captures.add(capture);
					}
					scopedProcess.end();
				}

				//from ProjectorFromStereoAndHelperCamera
				//----------
				void ProjectorFromGraycode::calibrate() {
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

					vector<glm::vec3> worldPoints;
					vector<glm::vec2> projectorImagePoints;

					auto selectedCaptures = this->captures.getSelection();
					for (auto capture : selectedCaptures) {
						worldPoints.insert(worldPoints.end(), capture->worldPoints.begin(), capture->worldPoints.end());
						projectorImagePoints.insert(projectorImagePoints.end(), capture->projectorImagePoints.begin(), capture->projectorImagePoints.end());
					}

					if (worldPoints.size() != projectorImagePoints.size()) {
						throw(ofxRulr::Exception("Size mismatch error between world points and projector points"));
					}

					auto count = worldPoints.size();
					if (count < 6) {
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

						auto flags = cv::CALIB_FIX_K1 | cv::CALIB_FIX_K2 | cv::CALIB_FIX_K3 | cv::CALIB_FIX_K4 | cv::CALIB_FIX_K5 | cv::CALIB_FIX_K6
							| cv::CALIB_ZERO_TANGENT_DIST | cv::CALIB_USE_INTRINSIC_GUESS | cv::CALIB_FIX_ASPECT_RATIO;
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
							if (glm::distance2(projectorImagePoints[i], ofxCv::toOf(reprojectedPoints[i])) < thresholdSquared) {
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
							cv::projectPoints(ofxCv::toCv(capture->worldPoints)
								, rotationMatrix
								, translation
								, cameraMatrix
								, distortionCoefficients
								, ofxCv::toCv(capture->reprojectedProjectorImagePoints));
						}
					}

					projectorNode->setExtrinsics(rotationMatrix, translation, true);
					projectorNode->setIntrinsics(cameraMatrix);
				}

				//----------
				void ProjectorFromGraycode::populateInspector(ofxCvGui::InspectArguments & inspectArgs) {
					auto inspector = inspectArgs.inspector;

					inspector->addButton("Add capture", [this]() {
						try {
							this->addCapture();
						}
						RULR_CATCH_ALL_TO_ALERT;
					}, ' ');
					this->captures.populateWidgets(inspector);
					inspector->addButton("Calibrate", [this]() {
						try {
							this->calibrate();
						}
						RULR_CATCH_ALL_TO_ALERT;
					}, OF_KEY_RETURN)->setHeight(100.0f);
					inspector->addLiveValue<float>(this->reprojectionError);
					inspector->addParameterGroup(this->parameters);
				}

				//----------
				void ProjectorFromGraycode::serialize(nlohmann::json & json) {
					Utils::serialize(json, this->parameters);
					this->captures.serialize(json["captures"]);
				}

				//----------
				void ProjectorFromGraycode::deserialize(const nlohmann::json & json) {
					Utils::deserialize(json, this->parameters);
					this->captures.deserialize(json["captures"]);
				}

				//----------
				void ProjectorFromGraycode::drawOnVideoOutput(const ofRectangle & bounds) {
					auto captures = this->captures.getSelection();
					for (const auto & capture : captures) {
						//draw data points as lines
						if (this->parameters.draw.dataOnVideoOutput) {
							ofPushStyle();
							{
								ofSetColor(capture->color);
								ofMesh line;
								line.setMode(ofPrimitiveMode::OF_PRIMITIVE_LINE_STRIP);
								for (auto projectorPoint : capture->projectorImagePoints) {
									line.addVertex({
										projectorPoint.x
										, projectorPoint.y
										, 0.0f
										});
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
								for (const auto reprojectedPoint : capture->reprojectedProjectorImagePoints) {
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
			}
		}
	}
}

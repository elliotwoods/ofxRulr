#include "pch_RulrNodes.h"
#include "StereoCalibrate.h"

#include "ofxRulr/Nodes/Item/Camera.h"
#include <future>

#include "ofxNonLinearFit.h"

using namespace ofxCv;
using namespace cv;

namespace ofxRulr {
	namespace Nodes {
		namespace Procedure {
			namespace Calibrate {
#pragma mark Capture
				//----------
				StereoCalibrate::Capture::Capture() {
					RULR_SERIALIZE_LISTENERS;
				}

				//----------
				string StereoCalibrate::Capture::getDisplayString() const {
					stringstream ss;
					ss << this->pointsImageSpaceA.size() << " points. " << endl;
					return ss.str();
				}

				//----------
				void StereoCalibrate::Capture::drawOnImageA() const {
					drawCorners(this->pointsImageSpaceA, false);

				}

				//----------
				void StereoCalibrate::Capture::drawOnImageB() const {
					drawCorners(this->pointsImageSpaceB, false);

				}

				//----------
				void StereoCalibrate::Capture::serialize(Json::Value & json) {
					json["pointsImageSpaceA"] << this->pointsImageSpaceA;
					json["pointsImageSpaceB"] << this->pointsImageSpaceB;
					json["pointsObjectSpace"] << this->pointsObjectSpace;
					json["pointsWorldSpace"] << this->pointsWorldSpace;
				}

				//----------
				void StereoCalibrate::Capture::deserialize(const Json::Value & json) {
					json["pointsImageSpaceA"] >> this->pointsImageSpaceA;
					json["pointsImageSpaceB"] >> this->pointsImageSpaceB;
					json["pointsObjectSpace"] >> this->pointsObjectSpace;
					json["pointsWorldSpace"] >> this->pointsWorldSpace;
				}

#pragma mark StereoCalibrate
				//----------
				StereoCalibrate::StereoCalibrate() {
					RULR_NODE_INIT_LISTENER;
				}

				//----------
				void StereoCalibrate::init() {
					RULR_NODE_UPDATE_LISTENER;
					RULR_NODE_INSPECTOR_LISTENER;
					RULR_NODE_SERIALIZATION_LISTENERS;
					RULR_NODE_DRAW_WORLD_LISTENER;

					this->addInput<Item::Camera>("Camera A");
					this->addInput<Item::Camera>("Camera B");
					this->addInput<Item::AbstractBoard>();

					//build gui
					{
						auto strip = ofxCvGui::Panels::Groups::makeStrip();
						{
							auto panel = ofxCvGui::Panels::makeImage(this->previewA);
							panel->onDrawImage += [this](ofxCvGui::DrawImageArguments & args) {
								//draw captures
								auto selectedCaptures = this->captures.getSelection();
								ofPushStyle();
								{
									for (auto & selectedCapture : selectedCaptures) {
										ofSetColor(selectedCapture->color);
										selectedCapture->drawOnImageA();
									}
								}
								ofPopStyle();

								//draw red on top if failed
								{
									auto timeSinceFailure = chrono::system_clock::now() - this->lastFailures.lastFailureA;
									if (timeSinceFailure < chrono::seconds(3)) {
										ofPushStyle();
										{
											ofEnableAlphaBlending();
											ofSetColor(255, 0, 0, ofMap(chrono::duration_cast<chrono::milliseconds>(timeSinceFailure).count(), 0, 3000, 255.0f, 0.0f));
											ofDrawRectangle(0, 0, args.drawSize.x, args.drawSize.y);
										}
										ofPopStyle();
									}
								}
							};
							strip->add(panel);
						}
						{
							auto panel = ofxCvGui::Panels::makeImage(this->previewB);
							panel->onDrawImage += [this](ofxCvGui::DrawImageArguments & args) {
								//draw captures
								auto selectedCaptures = this->captures.getSelection();
								ofPushStyle();
								{
									for (auto & selectedCapture : selectedCaptures) {
										ofSetColor(selectedCapture->color);
										selectedCapture->drawOnImageB();
									}
								}
								ofPopStyle();

								//draw red on top if failed
								{
									auto timeSinceFailure = chrono::system_clock::now() - this->lastFailures.lastFailureB;
									if (timeSinceFailure < chrono::seconds(3)) {
										ofPushStyle();
										{
											ofEnableAlphaBlending();
											ofSetColor(255, 0, 0, ofMap(chrono::duration_cast<chrono::milliseconds>(timeSinceFailure).count(), 0, 3000, 255.0f, 0.0f));
											ofDrawRectangle(0, 0, args.drawSize.x, args.drawSize.y);
										}
										ofPopStyle();
									}
								}
							};
							strip->add(panel);
						}
						this->view = strip;
					}
				}

				//----------
				string StereoCalibrate::getTypeName() const {
					return "Procedure::Calibrate::StereoCalibrate";
				}

				//----------
				ofxCvGui::PanelPtr StereoCalibrate::getPanel() {
					return this->view;
				}

				//----------
				void StereoCalibrate::update() {

				}

				//----------
				void StereoCalibrate::drawWorld() {
					if (this->parameters.draw.enabled) {
						auto captures = this->captures.getSelection();
						for (auto & capture : captures) {

							ofPushStyle();
							{
								ofSetColor(capture->color);

								//draw points
								if (this->parameters.draw.points) {
									for (auto & worldPoint : capture->pointsWorldSpace) {
										ofDrawSphere(worldPoint, 0.005f);
									}
								}

								//draw distances
								if (this->parameters.draw.distances) {
									set<pair<int, int>> distancesAlreadyDrawn; //index vs index
									for (int i = 0; i<capture->pointsWorldSpace.size(); i++) {
										//build a list of distances
										multimap<float, int> distancesSquared; //distance vs index
										for (int j = 0; j < capture->pointsWorldSpace.size(); j++) {
											if (j == i) {
												continue;
											}

											distancesSquared.emplace(capture->pointsWorldSpace[i].squareDistance(capture->pointsWorldSpace[j])
											, j);
										}

										//pick 2 closest
										int foundCount = 0;
										for (auto it = distancesSquared.begin(); it != distancesSquared.end(); it++) {
											//check we don't already have this one
											pair<int, int> distancePair(i, it->second);
											if (distancesAlreadyDrawn.find(distancePair) == distancesAlreadyDrawn.end()) {
												auto middle = (capture->pointsWorldSpace[distancePair.first] + capture->pointsWorldSpace[distancePair.second]) / 2.0f;
												ofDrawBitmapString(ofToString(sqrt(it->first)), middle);

												distancesAlreadyDrawn.insert(distancePair);
												foundCount++;
												if (foundCount >= 2) {
													break;
												}
											}
										}
									}
								}
							}
							ofPopStyle();
						}
					}
				}

				//----------
				void StereoCalibrate::serialize(Json::Value & json) {
					this->captures.serialize(json);
					Utils::Serializable::serialize(json, this->parameters);
					Utils::Serializable::serialize(json, this->reprojectionError);

					{
						const auto filename = this->getDefaultFilename() + "-opencvmatrices.yml";
						FileStorage file(filename, FileStorage::WRITE);
						file << "rotation" << this->openCVCalibration.rotation;
						file << "translation" << this->openCVCalibration.translation;
						file << "essential" << this->openCVCalibration.essential;
						file << "fundamental" << this->openCVCalibration.fundamental;
						file << "rectificationRotationA" << this->openCVCalibration.rectificationRotationA;
						file << "rectificationRotationB" << this->openCVCalibration.rectificationRotationB;
						file << "projectionA" << this->openCVCalibration.projectionA;
						file << "projectionB" << this->openCVCalibration.projectionB;
						file << "disparityToDepth" << this->openCVCalibration.disparityToDepth;
						file.release();
						json["opencvMatricesFile"] = filename;
					}
				}

				//----------
				void StereoCalibrate::deserialize(const Json::Value & json) {
					this->captures.deserialize(json);
					Utils::Serializable::deserialize(json, this->parameters);
					Utils::Serializable::deserialize(json, this->reprojectionError);

					if (json.isMember("opencvMatricesFile")) {
						auto filename = json["opencvMatricesFile"].asString();

						FileStorage file(filename, FileStorage::READ);
						file["rotation"] >> this->openCVCalibration.rotation;
						file["translation"] >> this->openCVCalibration.translation;
						file["essential"] >> this->openCVCalibration.essential;
						file["fundamental"] >> this->openCVCalibration.fundamental;
						file["rectificationRotationA"] >> this->openCVCalibration.rectificationRotationA;
						file["rectificationRotationB"] >> this->openCVCalibration.rectificationRotationB;
						file["projectionA"] >> this->openCVCalibration.projectionA;
						file["projectionB"] >> this->openCVCalibration.projectionB;
						file["disparityToDepth"] >> this->openCVCalibration.disparityToDepth;
						file.release();
					}
				}

				//----------
				const StereoCalibrate::OpenCVCalibration & StereoCalibrate::getOpenCVCalibration() const {
					return this->openCVCalibration;
				}

				//----------
				std::vector<ofVec3f> StereoCalibrate::triangulate(const vector<ofVec2f> & imagePointsA, const vector<ofVec2f> & imagePointsB, bool correctMatches) {
					//check that we have a calibration available
					if (this->openCVCalibration.rotation.empty()
						|| this->openCVCalibration.translation.empty()
						|| this->openCVCalibration.projectionA.empty()
						|| this->openCVCalibration.projectionB.empty()) {
						throw(ofxRulr::Exception("No stereo calibration available."));
					}

					//check that image sizes match
					if (imagePointsA.size() != imagePointsB.size()) {
						throw(ofxRulr::Exception("Cannot triangulate. Length of image point vectors do not match."));
					}

					auto cameraNodeA = this->getInput<Item::Camera>("Camera A");
					auto cameraNodeB = this->getInput<Item::Camera>("Camera B");

					if (!cameraNodeA || !cameraNodeB) {
						throw(ofxRulr::Exception("Cannot triangulate. Cameras is not attached."));
					}

					auto cameraMatrixA = cameraNodeA->getCameraMatrix();
					auto cameraMatrixB = cameraNodeB->getCameraMatrix();
					auto distortionCoefficientsA = cameraNodeA->getDistortionCoefficients();
					auto distortionCoefficientsB = cameraNodeB->getDistortionCoefficients();

					//build up the data
					vector<cv::Point2d> projectedPointsA, projectedPointsB;
					cv::Mat pointsInImageSpace64FA, pointsInImageSpace64FB;
					cv::Mat(ofxCv::toCv(imagePointsA)).convertTo(pointsInImageSpace64FA, CV_64FC2);
					cv::Mat(ofxCv::toCv(imagePointsB)).convertTo(pointsInImageSpace64FB, CV_64FC2);

					//undistort the points
					cv::undistortPoints(pointsInImageSpace64FA
						, projectedPointsA
						, cameraMatrixA
						, distortionCoefficientsA);
					cv::undistortPoints(pointsInImageSpace64FB
						, projectedPointsB
						, cameraMatrixB
						, distortionCoefficientsB);

					//from normalized (ideal) coordinates back into image space
					{
						for (auto & point : projectedPointsA) {
							point.x = point.x * cameraMatrixA.at<double>(0, 0) + cameraMatrixA.at<double>(0, 2);
							point.y = point.y * cameraMatrixA.at<double>(1, 1) + cameraMatrixA.at<double>(1, 2);
						}
						for (auto & point : projectedPointsB) {
							point.x = point.x * cameraMatrixB.at<double>(0, 0) + cameraMatrixB.at<double>(0, 2);
							point.y = point.y * cameraMatrixB.at<double>(1, 1) + cameraMatrixB.at<double>(1, 2);
						}
					}

					if (correctMatches) {
						cv::correctMatches(this->openCVCalibration.fundamental
							, projectedPointsA
							, projectedPointsB
							, projectedPointsA
							, projectedPointsB);
					}

					size_t pointCount = imagePointsA.size();

					cv::Mat worldPoints(pointCount, 4, CV_64FC1);
					cv::triangulatePoints(this->openCVCalibration.projectionA
						, this->openCVCalibration.projectionB
						, projectedPointsA
						, projectedPointsB
						, worldPoints);

					vector<ofVec3f> worldPointsOutput;
					for (int i = 0; i < pointCount; i++) {
						cv::Vec4d worldPoint(worldPoints.at<double>(0, i)
							, worldPoints.at<double>(1, i)
							, worldPoints.at<double>(2, i)
							, worldPoints.at<double>(3, i));
						worldPoint /= worldPoint[3];
						worldPointsOutput.emplace_back(worldPoint[0], worldPoint[1], worldPoint[2]);
					}

					return worldPointsOutput;
				}

				struct SolvePnPDataPoint {
					vector<cv::Point3f> objectSpacePoint;
					vector<cv::Point2f> imagePointProjectionA;
					vector<cv::Point2f> imagePointProjectionB;
				};

				class SolvePnPModel : public ofxNonLinearFit::Models::Base<SolvePnPDataPoint, SolvePnPModel> {
				public:
					struct System {
						cv::Mat cameraMatrixA;
						cv::Mat cameraMatrixB;
						cv::Mat distortionCoefficientsA;
						cv::Mat distortionCoefficientsB;
						cv::Mat rotationVectorStereo;
						cv::Mat translationStereo;
					};

					SolvePnPModel() {
						this->rotationVectorA = cv::Mat(3, 1, CV_64F);
						this->rotationVectorA = cv::Mat(3, 1, CV_64F);
						this->translationB = cv::Mat(3, 1, CV_64F);
						this->translationB = cv::Mat(3, 1, CV_64F);
					}

					unsigned int getParameterCount() const override {
						return 6;
					}

					double getResidual(SolvePnPDataPoint dataPoint) const override {
						auto dataPointTest = dataPoint;
						this->evaluate(dataPointTest);

						double residualSquaredSum = 0.0;
						for (int i = 0; i < dataPoint.objectSpacePoint.size(); i++) {
							residualSquaredSum += ofxCv::toOf(dataPoint.imagePointProjectionA[i]).squareDistance(ofxCv::toOf(dataPointTest.imagePointProjectionA[i]));
						}
						return sqrt(residualSquaredSum);
					}

					void evaluate(SolvePnPDataPoint & dataPoint) const override {
						cv::projectPoints(dataPoint.objectSpacePoint
							, this->rotationVectorA
							, this->translationA
							, this->system.cameraMatrixA
							, this->system.distortionCoefficientsA
							, dataPoint.imagePointProjectionA);

						cv::projectPoints(dataPoint.objectSpacePoint
							, this->rotationVectorB
							, this->translationB
							, this->system.cameraMatrixB
							, this->system.distortionCoefficientsB
							, dataPoint.imagePointProjectionB);
					}
					
					virtual void cacheModel() override {
						this->rotationVectorA.at<double>(0) = this->parameters[0];
						this->rotationVectorA.at<double>(1) = this->parameters[1];
						this->rotationVectorA.at<double>(2) = this->parameters[2];
						this->translationA.at<double>(0) = this->parameters[3];
						this->translationA.at<double>(1) = this->parameters[4];
						this->translationA.at<double>(2) = this->parameters[5];

						cv::composeRT(this->rotationVectorA
							, this->translationA
							, this->system.rotationVectorStereo
							, this->system.translationStereo
							, this->rotationVectorB
							, this->translationB);
					}

					System system;

					cv::Mat rotationVectorA;
					cv::Mat rotationVectorB;
					cv::Mat translationA;
					cv::Mat translationB;
				};

				//----------
				//hacked from https://github.com/opencv/opencv/blob/3.1.0/modules/calib3d/src/calibration.cpp#L1154
				bool StereoCalibrate::solvePnP(const vector<cv::Point2f> & imagePointsA, const vector<cv::Point2f> & imagePointsB, const vector<cv::Point3f> & objectPoints, cv::Mat rotationVector, cv::Mat translation, bool useExtrinsicGuess /*= true*/) {
					auto count = imagePointsA.size();
					if (imagePointsB.size() != count
						|| objectPoints.size() != count) {
						throw(ofxRulr::Exception("solvePnP requires vectors with equal length as input"));
					}
					this->throwIfACameraIsDisconnected();

					//build the model
					SolvePnPModel model;
					{
						auto cameraNodeA = this->getInput<Item::Camera>("Camera A");
						auto cameraNodeB = this->getInput<Item::Camera>("Camera B");

						model.system = SolvePnPModel::System { cameraNodeA->getCameraMatrix()
							, cameraNodeB->getCameraMatrix()
							, cameraNodeA->getDistortionCoefficients()
							, cameraNodeB->getDistortionCoefficients()
							, this->openCVCalibration.rotation
							, this->openCVCalibration.translation
						};
					}
					
					ofxNonLinearFit::Fit<SolvePnPModel> fit;

					//build the dataSet
					vector<SolvePnPDataPoint> dataSet;
					{
						SolvePnPDataPoint dataPoint;
						dataPoint.imagePointProjectionA = imagePointsA;
						dataPoint.imagePointProjectionB = imagePointsB;
						dataPoint.objectSpacePoint = objectPoints;
					}

					//perform the fit
					double residual = 0.0;
					auto success = fit.optimise(model, &dataSet, &residual);

					//TODO
					// * build a test case (compare to opencv's solvePnP)
					// * objectPoints don't need to be the same for both cameras
					// * check transforms are correct order + inversion
					// * add error from second camera (+check the order + inversion)
					// * consider using Jacobian
					rotationVector = model.rotationVectorA;
					translation = model.translationA;

					return success;
				}

				//----------
				void StereoCalibrate::throwIfACameraIsDisconnected() {
					if (!this->getInputPin<Item::Camera>("Camera A")->isConnected()) {
						throw(ofxRulr::Exception("Camera A is disconnected"));
					}
					if (!this->getInputPin<Item::Camera>("Camera B")->isConnected()) {
						throw(ofxRulr::Exception("Camera B is disconnected"));
					}
				}

				//----------
				void StereoCalibrate::populateInspector(ofxCvGui::InspectArguments & inspectArgs) {
					auto inspector = inspectArgs.inspector;

					inspector->addTitle("Captures", ofxCvGui::Widgets::Title::H2);
					this->captures.populateWidgets(inspector);
					inspector->addButton("Add capture", [this]() {
						try {
							Utils::ScopedProcess scopedProcess("Add capture");
							this->addCapture();
							scopedProcess.end();
						}
						RULR_CATCH_ALL_TO_ERROR;
					}, ' ');
					inspector->addButton("Calibrate", [this]() {
						try {
							Utils::ScopedProcess scopedProcess("Calibrate");
							this->calibrate();
							scopedProcess.end();
						}
						RULR_CATCH_ALL_TO_ALERT;
					}, OF_KEY_RETURN)->setHeight(100.0f);
					inspector->addLiveValue<float>(this->reprojectionError);

					inspector->addParameterGroup(this->parameters);
				}

				//----------
				void StereoCalibrate::addCapture() {
					this->throwIfMissingAnyConnection();

					auto boardNode = this->getInput<Item::AbstractBoard>();
					auto cameraNodeA = this->getInput<Item::Camera>("Camera A");
					auto cameraNodeB = this->getInput<Item::Camera>("Camera B");

					auto cameraFrameA = cameraNodeA->getFrame();
					auto cameraFrameB = cameraNodeB->getFrame();

					//check we got frames
					{
						if (!cameraFrameA) {
							throw(ofxRulr::Exception("No frame for camera A"));
						}
						if (!cameraFrameB) {
							throw(ofxRulr::Exception("No frame for camera B"));
						}
					}


					//update previews
					{
						this->previewA = cameraFrameA->getPixels();
						this->previewB = cameraFrameB->getPixels();
					
					}
					//build capture
					{
						auto capture = make_shared<Capture>();

						auto imageA = ofxCv::toCv(cameraFrameA->getPixels());
						auto imageB = ofxCv::toCv(cameraFrameB->getPixels());

						vector<cv::Point2f> imagePointsA;
						vector<cv::Point2f> imagePointsB;
						vector<cv::Point3f> objectPointsA;
						vector<cv::Point3f> objectPointsB;

						//find the board in both cameras
						{
							auto findBoardMode = this->parameters.capture.findBoardMode.get();
							auto futureA = std::async(std::launch::async, [&] {
								return boardNode->findBoard(imageA
									, imagePointsA
									, objectPointsA
									, findBoardMode
									, cameraNodeA->getCameraMatrix()
									, cameraNodeA->getDistortionCoefficients());
							});
							auto futureB = std::async(std::launch::async, [&] {
								return boardNode->findBoard(imageB
									, imagePointsB
									, objectPointsB
									, findBoardMode
									, cameraNodeB->getCameraMatrix()
									, cameraNodeB->getDistortionCoefficients());
							});

							if (!futureA.get()) {
								this->lastFailures.lastFailureA = chrono::system_clock::now();
							}
							if (!futureB.get()) {
								this->lastFailures.lastFailureB = chrono::system_clock::now();
							}
						}

						Item::AbstractBoard::filterCommonPoints(imagePointsA
							, imagePointsB
							, objectPointsA
							, objectPointsB);
						
						if (imagePointsA.empty()) {
							throw(ofxRulr::Exception("No common board points found between 2 cameras"));
						}

						capture->pointsImageSpaceA = ofxCv::toOf(imagePointsA);
						capture->pointsImageSpaceB = ofxCv::toOf(imagePointsB);
						capture->pointsObjectSpace = ofxCv::toOf(objectPointsA);

						this->captures.add(capture);
					}
				}

				//----------
				void StereoCalibrate::calibrate() {
					auto cameraNodeA = this->getInput<Item::Camera>("Camera A");
					auto cameraNodeB = this->getInput<Item::Camera>("Camera B");

					if (!cameraNodeA || !cameraNodeB) {
						throw(ofxRulr::Exception("Both camera nodes must be connected to perform stereo calibration"));
					}

					if (cameraNodeA->getSize() != cameraNodeB->getSize()) {
						throw(ofxRulr::Exception("Both cameras must have equal resolution"));
					}

					vector<vector<cv::Point2f>> imagePointsA, imagePointsB;
					vector<vector<cv::Point3f>> objectPoints;

					auto selectedCaptures = this->captures.getSelection();
					for (auto capture : selectedCaptures) {
						imagePointsA.push_back(ofxCv::toCv(capture->pointsImageSpaceA));
						imagePointsB.push_back(ofxCv::toCv(capture->pointsImageSpaceB));
						objectPoints.push_back(ofxCv::toCv(capture->pointsObjectSpace));
					}

					cv::Mat rotation, translation;
					cv::Mat essential, fundamental;
					cv::Mat cameraMatrixA = cameraNodeA->getCameraMatrix();
					cv::Mat cameraMatrixB = cameraNodeB->getCameraMatrix();
					cv::Mat distortionCoefficientsA = cameraNodeA->getDistortionCoefficients();
					cv::Mat distortionCoefficientsB = cameraNodeB->getDistortionCoefficients();

					int flags = 0;
					if (!this->parameters.calibration.fixIntrinsics) {
						flags |= CV_CALIB_USE_INTRINSIC_GUESS;
					}
					else {
						flags |= CV_CALIB_FIX_INTRINSIC;
					}

					this->reprojectionError = cv::stereoCalibrate(objectPoints
						, imagePointsA
						, imagePointsB
						, cameraMatrixA
						, distortionCoefficientsA
						, cameraMatrixB
						, distortionCoefficientsB
						, cameraNodeA->getSize()
						, rotation
						, translation
						, essential
						, fundamental
						, flags);

					if (!this->parameters.calibration.fixIntrinsics) {
						cameraNodeA->setIntrinsics(cameraMatrixA, distortionCoefficientsA);
						cameraNodeB->setIntrinsics(cameraMatrixB, distortionCoefficientsB);
					}

					auto transformBToA = ofxCv::makeMatrix(rotation, translation);
					cameraNodeB->setTransform(transformBToA.getInverse() * cameraNodeA->getTransform());

					//create the rectified data
					cv::Mat rectificationRotationA, rectificationRotationB;
					cv::Mat projectionA, projectionB;
					cv::Mat disparityToDepth;
					{
						cv::stereoRectify(cameraMatrixA
							, distortionCoefficientsA
							, cameraMatrixB
							, distortionCoefficientsB
							, cameraNodeA->getSize()
							, rotation
							, translation
							, rectificationRotationA
							, rectificationRotationB
							, projectionA
							, projectionB
							, disparityToDepth
						);
					}

					//save our 'OpenCVCalibration'
					this->openCVCalibration = OpenCVCalibration{ rotation
						, translation
						, essential
						, fundamental
						, rectificationRotationA
						, rectificationRotationB
						, projectionA
						, projectionB
						, disparityToDepth
					};

					//triangulate the points to see their world space re-projections
					for (auto capture : selectedCaptures) {
						capture->pointsWorldSpace = this->triangulate(capture->pointsImageSpaceA, capture->pointsImageSpaceB, false);
					}
				}
			}
		}
	}
}
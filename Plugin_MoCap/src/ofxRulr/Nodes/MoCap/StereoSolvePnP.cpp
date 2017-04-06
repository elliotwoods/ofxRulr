#include "pch_Plugin_MoCap.h"
#include "StereoSolvePnP.h"

#include "ofxRulr/Nodes/Procedure/Calibrate/StereoCalibrate.h"
#include "ofxRulr/Nodes/Item/Camera.h"
#include "ofxRulr/Nodes/Item/AbstractBoard.h"

namespace ofxRulr {
	namespace Nodes {
		namespace MoCap {

#pragma mark SolvePnPDataPoint
			struct SolvePnPDataPoint {
				vector<cv::Point3f> objectSpacePointsA;
				vector<cv::Point3f> objectSpacePointsB;
				vector<cv::Point2f> imagePointProjectionsA;
				vector<cv::Point2f> imagePointProjectionsB;
			};

#pragma mark SolvePnPModel
			class SolvePnPModel : public ofxNonLinearFit::Models::Base<SolvePnPDataPoint, SolvePnPModel> {
			public:
				struct System {
					cv::Mat initialRotationVector;
					cv::Mat initialTranslation;

					cv::Mat cameraMatrixA;
					cv::Mat cameraMatrixB;
					cv::Mat distortionCoefficientsA;
					cv::Mat distortionCoefficientsB;

					cv::Mat rotationVectorStereoInverse;
					cv::Mat translationStereoInverse;
				};

				SolvePnPModel() {
					this->rotationVectorA = cv::Mat(3, 1, CV_64F, double(0.0));
					this->rotationVectorB = cv::Mat(3, 1, CV_64F, double(0.0));
					this->translationA = cv::Mat(3, 1, CV_64F, double(0.0));
					this->translationB = cv::Mat(3, 1, CV_64F, double(0.0));
				}

				unsigned int getParameterCount() const override {
					return 6;
				}

				void getResidual(SolvePnPDataPoint dataPoint, double & residual, double * gradient) const override {
					auto dataPointTest = dataPoint;
					this->evaluate(dataPointTest);

					double residualSum = 0.0;
					cv::Mat gradientSum(1, 6, CV_64F, 0.0);

					for (int i = 0; i < dataPoint.objectSpacePointsA.size(); i++) {
						auto delta = dataPoint.imagePointProjectionsA[i] - dataPointTest.imagePointProjectionsA[i];

						if (gradient) {
							auto dx_drA = this->jacobian.dpA_drA(cv::Rect(0, i * 2 + 0, 3, 1));
							auto dy_drA = this->jacobian.dpA_drA(cv::Rect(0, i * 2 + 1, 3, 1));

							auto dx_dtA = this->jacobian.dpA_dtA(cv::Rect(0, i * 2 + 0, 3, 1));
							auto dy_dtA = this->jacobian.dpA_dtA(cv::Rect(0, i * 2 + 1, 3, 1));

							auto pointGradientRotation =
								+ (delta.x / abs(delta.x)) * dx_drA
								+ (delta.y / abs(delta.y)) * dy_drA;

							auto pointGradientTranslation =
								+ (delta.x / abs(delta.x)) * dx_dtA
								+ (delta.y / abs(delta.y)) * dy_dtA;

							cv::Mat pointGradient;
							cv::hconcat(pointGradientRotation, pointGradientTranslation, pointGradient);
							gradientSum -= pointGradient;
						}

						residualSum += abs(delta.x) + abs(delta.y);
					}

					for (int i = 0; i < dataPoint.objectSpacePointsB.size(); i++) {
						auto delta = dataPoint.imagePointProjectionsB[i] - dataPointTest.imagePointProjectionsB[i];

						if (gradient) {
							auto dx_drA = this->jacobian.dpB_drA(cv::Rect(0, i * 2 + 0, 3, 1));
							auto dy_drA = this->jacobian.dpB_drA(cv::Rect(0, i * 2 + 1, 3, 1));

							auto dx_dtA = this->jacobian.dpB_dtA(cv::Rect(0, i * 2 + 0, 3, 1));
							auto dy_dtA = this->jacobian.dpB_dtA(cv::Rect(0, i * 2 + 1, 3, 1));

							auto pointGradientRotation =
								+(delta.x / abs(delta.x)) * dx_drA
								+ (delta.y / abs(delta.y)) * dy_drA;

							auto pointGradientTranslation =
								+(delta.x / abs(delta.x)) * dx_dtA
								+ (delta.y / abs(delta.y)) * dy_dtA;

							cv::Mat pointGradient;
							cv::hconcat(pointGradientRotation, pointGradientTranslation, pointGradient);
						}

						residualSum += abs(delta.x) + abs(delta.y);
					}

					auto totalPointCount = dataPoint.objectSpacePointsA.size() + dataPoint.objectSpacePointsB.size();
					residual = residualSum / (double)totalPointCount;
					if (gradient) {
						auto gradientAverage = gradientSum / (double) totalPointCount;
						memcpy(gradient, gradientSum.data, 6 * sizeof(double));
					}
				}

				void evaluate(SolvePnPDataPoint & dataPoint) const override {
					//jacobian for first view (presumed to have no transform)
					{
						cv::Mat dpB_drtA;
						cv::projectPoints(dataPoint.objectSpacePointsA
							, this->rotationVectorA
							, this->translationA
							, this->system.cameraMatrixA
							, this->system.distortionCoefficientsA
							, dataPoint.imagePointProjectionsA
							, dpB_drtA);
						this->jacobian.dpA_drA = dpB_drtA(cv::Rect(0, 0, 3, dataPoint.objectSpacePointsA.size() * 2));
						this->jacobian.dpA_dtA = dpB_drtA(cv::Rect(3, 0, 3, dataPoint.objectSpacePointsA.size() * 2));
					}

					cv::Mat dpB_drtB;
					cv::projectPoints(dataPoint.objectSpacePointsB
						, this->rotationVectorB
						, this->translationB
						, this->system.cameraMatrixB
						, this->system.distortionCoefficientsB
						, dataPoint.imagePointProjectionsB
						, dpB_drtB);

					//transform Jacobian into first view
					{
						cv::Mat dpB_drA;
						cv::Mat dpB_dtA;

						for (int i = 0; i < dataPoint.objectSpacePointsB.size(); i++) {
							//x coord
							{
								//get gradients in camera B pose parameters
								cv::Mat dpBx_drB(3, 1, CV_64F);
								{
									dpBx_drB.at<double>(0) = dpB_drtB.at<double>(0, i * 2 + 0);
									dpBx_drB.at<double>(1) = dpB_drtB.at<double>(1, i * 2 + 0);
									dpBx_drB.at<double>(2) = dpB_drtB.at<double>(2, i * 2 + 0);
								}
								cv::Mat dpBx_dtB(3, 1, CV_64F);
								{
									dpBx_dtB.at<double>(0) = dpB_drtB.at<double>(3, i * 2 + 0);
									dpBx_dtB.at<double>(1) = dpB_drtB.at<double>(4, i * 2 + 0);
									dpBx_dtB.at<double>(2) = dpB_drtB.at<double>(5, i * 2 + 0);
								}

								//transfer gradients into camera A pose parameters
								auto dpBx_drA = this->transferJacobian.drB_drA * dpBx_drB
									+ this->transferJacobian.dtB_drA * dpBx_dtB;
								auto dpBx_dtA = this->transferJacobian.drB_dtA * dpBx_drB
									+ this->transferJacobian.dtB_dtA * dpBx_dtB;

								//add results to big jacobian
								{
									cv::Mat dpBx_drAT;
									cv::transpose(dpBx_drA, dpBx_drAT);
									if (dpB_drA.empty()) {
										//first row
										dpB_drA = dpBx_drAT;
									}
									else {
										cv::vconcat(dpB_drA, dpBx_drAT, dpB_drA);
									}

									cv::Mat dpBx_dtAT;
									cv::transpose(dpBx_dtA, dpBx_dtAT);
									if (dpB_dtA.empty()) {
										//first row
										dpB_dtA = dpBx_dtAT;
									}
									else {
										cv::vconcat(dpB_dtA, dpBx_dtAT, dpB_dtA);
									}
								}
							}

							//y coord
							{
								//get gradients in camera B pose parameters
								cv::Mat dpBy_drB(3, 1, CV_64F);
								{
									dpBy_drB.at<double>(0) = dpB_drtB.at<double>(0, i * 2 + 1);
									dpBy_drB.at<double>(1) = dpB_drtB.at<double>(1, i * 2 + 1);
									dpBy_drB.at<double>(2) = dpB_drtB.at<double>(2, i * 2 + 1);

								}
								cv::Mat dpBy_dtB(3, 1, CV_64F);
								{
									dpBy_dtB.at<double>(0) = dpB_drtB.at<double>(3, i * 2 + 1);
									dpBy_dtB.at<double>(1) = dpB_drtB.at<double>(4, i * 2 + 1);
									dpBy_dtB.at<double>(2) = dpB_drtB.at<double>(5, i * 2 + 1);
								}

								//transfer gradients into camera A pose parameters
								auto dpBy_drA = this->transferJacobian.drB_drA * dpBy_drB
									+ this->transferJacobian.dtB_drA * dpBy_dtB;
								auto dpBy_dtA = this->transferJacobian.drB_dtA * dpBy_drB
									+ this->transferJacobian.dtB_dtA * dpBy_dtB;

								//add results to big jacobian
								{
									cv::Mat dpBy_drAT;
									cv::transpose(dpBy_drA, dpBy_drAT);
									cv::vconcat(dpB_drA, dpBy_drAT, dpB_drA);

									cv::Mat dpBy_dtAT;
									cv::transpose(dpBy_dtA, dpBy_dtAT);
									cv::vconcat(dpB_dtA, dpBy_dtAT, dpB_dtA);
								}
							}
						}

						this->jacobian.dpB_drA = move(dpB_drA);
						this->jacobian.dpB_dtA = move(dpB_dtA);
					}
				}

				virtual void cacheModel() override {
					this->rotationVectorA.at<double>(0) = this->parameters[0];
					this->rotationVectorA.at<double>(1) = this->parameters[1];
					this->rotationVectorA.at<double>(2) = this->parameters[2];
					this->translationA.at<double>(0) = this->parameters[3];
					this->translationA.at<double>(1) = this->parameters[4];
					this->translationA.at<double>(2) = this->parameters[5];

					cv::Mat jacobianTransfer;

					cv::composeRT(this->rotationVectorA
						, this->translationA
						, this->system.rotationVectorStereoInverse
						, this->system.translationStereoInverse
						, this->rotationVectorB
						, this->translationB

						//Jacobian's
						, this->transferJacobian.drB_drA
						, this->transferJacobian.drB_dtA
						, this->transferJacobian.drB_drS
						, this->transferJacobian.drB_dtS
						, this->transferJacobian.dtB_drA
						, this->transferJacobian.dtB_dtA
						, this->transferJacobian.dtB_drS
						, this->transferJacobian.dtB_dtS);
				}

				System system;

				cv::Mat rotationVectorA;
				cv::Mat rotationVectorB;
				cv::Mat translationA;
				cv::Mat translationB;

				struct {
					bool ready = false;
					cv::Mat drB_drA;
					cv::Mat drB_dtA;
					cv::Mat drB_drS; //unused
					cv::Mat drB_dtS; //unused
					cv::Mat dtB_drA;
					cv::Mat dtB_dtA;
					cv::Mat dtB_drS; //unused
					cv::Mat dtB_dtS; //unused
				} transferJacobian;

				struct {
					mutable cv::Mat dpA_drA;
					mutable cv::Mat dpA_dtA;

					mutable cv::Mat dpB_drA;
					mutable cv::Mat dpB_dtA;
				} jacobian;

				//----------
				virtual void resetParameters() override {
					this->parameters[0] = this->system.initialRotationVector.at<double>(0);
					this->parameters[1] = this->system.initialRotationVector.at<double>(1);
					this->parameters[2] = this->system.initialRotationVector.at<double>(2);
					this->parameters[3] = this->system.initialTranslation.at<double>(0);
					this->parameters[4] = this->system.initialTranslation.at<double>(1);
					this->parameters[5] = this->system.initialTranslation.at<double>(2);
				}
			};


#pragma mark solvePnPStereo function
			//----------
			//hacked from https://github.com/opencv/opencv/blob/3.1.0/modules/calib3d/src/calibration.cpp#L1154
			//temporarily make it a non-static function to export debug info
			bool StereoSolvePnP::solvePnPStereo(shared_ptr<Procedure::Calibrate::StereoCalibrate> stereoCalibrateNode
				, const vector<cv::Point2f> & imagePointsA
				, const vector<cv::Point2f> & imagePointsB
				, const vector<cv::Point3f> & objectPointsA
				, const vector<cv::Point3f> & objectPointsB
				, cv::Mat & rotationVector
				, cv::Mat & translation
				, bool useExtrinsicGuess) {

				//check vectors are of equal length
				if (imagePointsA.size() != objectPointsA.size()
					|| imagePointsB.size() != objectPointsB.size()) {
					throw(ofxRulr::Exception("solvePnPStereo requires sets of image points and object points with equal length per camera."));
				}
				
				//check we have enough data to continue
				{
					if (imagePointsA.size() >= 4
						|| imagePointsB.size() >= 4) {
						//OK
					}
					else {
						auto commonImagePointsA = imagePointsA;
						auto commonImagePointsB = imagePointsB;
						auto commonObjectPointsA = objectPointsA;
						auto commonObjectPointsB = objectPointsB;

						Item::AbstractBoard::filterCommonPoints(commonImagePointsA
							, commonImagePointsB
							, commonObjectPointsA
							, commonObjectPointsB);

						auto commonPointsCount = commonImagePointsA.size();
						auto uncommonPointCount = (imagePointsA.size() - commonPointsCount)
							+ (imagePointsB.size() - commonPointsCount);
						if (commonPointsCount + uncommonPointCount >= 3) {
							//OK
						} else {
							throw(ofxRulr::Exception("Not enough data points to perform stereo solvePnP"));
						}
					}
				}

				//get the stereo calibration and check cameras connected
				stereoCalibrateNode->throwIfACameraIsDisconnected();
				auto openCVCalibration = stereoCalibrateNode->getOpenCVCalibration();

				//get inverse transform between 2 views
				cv::Mat rotationVectorStereoInverse;
				cv::Mat translationStereoInverse;
				{
					auto stereoTransform = ofxCv::makeMatrix(openCVCalibration.rotationVector, openCVCalibration.translation);
					auto stereoTransformInverse = stereoTransform.getInverse();
					ofxCv::decomposeMatrix(stereoTransformInverse, rotationVectorStereoInverse, translationStereoInverse);
				}

				//HACK in either of these cases we disable useExtrinsicGuess
				if (imagePointsA.empty() || imagePointsB.empty()) {
					//we do this because we want to use the single view procedure in this case (which is what we use to make our guess)

					useExtrinsicGuess = false;
				}

				//build the model
				SolvePnPModel model;
				bool hasInitialGuess = false;
				{
					auto cameraNodeA = stereoCalibrateNode->getInput<Item::Camera>("Camera A");
					auto cameraNodeB = stereoCalibrateNode->getInput<Item::Camera>("Camera B");

					if (useExtrinsicGuess) {
						hasInitialGuess = true;
					} else {
						//initialise the vectors if no existing data by using single-camera result with lowest reprojection error
						//also if we only have one camera, then we'll use this result anyway
						size_t countSum = 0;

						struct Result {
							cv::Mat rotationVector;
							cv::Mat translation;
							float reprojectionError;
						};

						Result bestResult{ cv::Mat()
							, cv::Mat()
							, numeric_limits<float>::max()
						};
						mutex bestResultMutex;

						//function to perform solvePnP using a single camera (and to check its result)
						auto tryCameraPnP = [& bestResult, &bestResultMutex](const cv::Mat & cameraMatrix
							, const cv::Mat & distortionCoefficients
							, const cv::Mat & cameraRotationInverse
							, const cv::Mat & cameraTranslationInverse
							, const vector<cv::Point3f> & objectPoints
							, const vector<cv::Point2f> & imagePoints) {
							if (imagePoints.size() < 4) {
								return;
							}

							cv::Mat rotationVector;
							cv::Mat translation;

							//calculate RMS error
							float errorRMS;
							{
								cv::solvePnP(objectPoints
									, imagePoints
									, cameraMatrix
									, distortionCoefficients
									, rotationVector
									, translation
									, false);

								vector<cv::Point2f> reprojected;

								cv::projectPoints(objectPoints
									, rotationVector
									, translation
									, cameraMatrix
									, distortionCoefficients
									, reprojected);

								float errorSquaredSum = 0.0f;
								for (int i = 0; i < imagePoints.size(); i++) {
									auto delta = reprojected[i] - imagePoints[i];
									errorSquaredSum += delta.dot(delta);
								}
								errorRMS = sqrt(errorSquaredSum);
							}
								
							//check if our solution is best so far
							if (errorRMS < bestResult.reprojectionError) {
								//if so set ours as best

								//first cancel out camera rotation, translation
								if (!cameraRotationInverse.empty()) {
									cv::composeRT(rotationVector
										, translation
										, cameraRotationInverse
										, cameraTranslationInverse
										, rotationVector
										, translation);
								}

								//save the result
								{
									auto lock = unique_lock<mutex>(bestResultMutex);
									bestResult = Result{ rotationVector
										, translation
										, errorRMS
									};
								}
							}
						};

						auto futureA = std::async(std::launch::async, [&] {
							tryCameraPnP(cameraNodeA->getCameraMatrix()
								, cameraNodeA->getDistortionCoefficients()
								, cv::Mat()
								, cv::Mat()
								, objectPointsA
								, imagePointsA);
						});

						auto futureB = std::async(std::launch::async, [&] {
							tryCameraPnP(cameraNodeB->getCameraMatrix()
								, cameraNodeB->getDistortionCoefficients()
								, rotationVectorStereoInverse
								, translationStereoInverse
								, objectPointsB
								, imagePointsB);
						});

						futureA.get();
						futureB.get();

						if (bestResult.reprojectionError < numeric_limits<float>::max()) {
							//start with our best result so far
							rotationVector = move(bestResult.rotationVector);
							translation = move(bestResult.translation);
							hasInitialGuess = true;
						}
						else {
							//start with zeros
							rotationVector = cv::Mat(3, 1, 0.0f);
							translation = cv::Mat(3, 1, 0.0f);
							hasInitialGuess = false;
						}
					}

					//initialise the system
					model.system = SolvePnPModel::System{
						rotationVector
						, translation
						, cameraNodeA->getCameraMatrix()
						, cameraNodeB->getCameraMatrix()
						, cameraNodeA->getDistortionCoefficients()
						, cameraNodeB->getDistortionCoefficients()
						, rotationVectorStereoInverse
						, translationStereoInverse
					};

					model.initialiseParameters();
				}

				//build the dataSet
				vector<SolvePnPDataPoint> dataSet;
				{
					SolvePnPDataPoint dataPoint;
					dataPoint.imagePointProjectionsA = imagePointsA;
					dataPoint.imagePointProjectionsB = imagePointsB;
					dataPoint.objectSpacePointsA = objectPointsA;
					dataPoint.objectSpacePointsB = objectPointsB;
					dataSet.emplace_back(dataPoint);
				}

				bool success = false;

				if (imagePointsA.empty() || imagePointsB.empty()) {
					//HACK : in this case we forced useExtrinsicGuess to false, so we've already calculated a result
					if (hasInitialGuess) {
						rotationVector = model.system.initialRotationVector;
						translation = model.system.initialTranslation;
						success = true;
					}
					else {
						return false;
					}
				}
				else {
					//We have points in both cameras, let's do the thing!!
					double residual = 0.0;
					ofxNonLinearFit::Fit<SolvePnPModel> fit(ofxNonLinearFit::Algorithm(nlopt::LD_TNEWTON, ofxNonLinearFit::Algorithm::Domain::LocalGradientless));
					{
						auto & optimiser = fit.getOptimiser();
						
						{
							double lowerBounds[6];
							for(int i=0; i<6; i++) {
								lowerBounds[i] = model.getParameters()[i] - ((i < 1) ? PI : 1);
							};
							nlopt_set_lower_bounds(optimiser, lowerBounds);
						}
						{
							double upperBounds[6];
							for (int i = 0; i < 6; i++) {
								upperBounds[i] = model.getParameters()[i] + ((i < 1) ? PI : 1);
							};
							nlopt_set_upper_bounds(optimiser, upperBounds);
						}
						{
							const auto angleTolerance = 1e-2 * PI;
							const auto positionTolerance = 1e-3;

							double absoluteTolerance[6] = {
								angleTolerance
								, angleTolerance
								, angleTolerance
								, positionTolerance
								, positionTolerance
								, positionTolerance
							};
							nlopt_set_xtol_abs(optimiser, absoluteTolerance);
						}
						nlopt_set_maxtime(optimiser, 1.0 / 200.0);
					}


					double residualBefore;
					model.getResidualOnSet(dataSet, residualBefore, NULL);

					//HACK
					//success = fit.optimise(model, &dataSet, &residual);
					success = true;

					model.cacheModel();

					success &= !isnan(residual);

					if (success) {
						//cout << "Residual : " << residualBefore << " -> " << residual << endl;
						//double residualEvaluated;
						//model.getResidualOnSet(dataSet, residualEvaluated, NULL);

						rotationVector = model.rotationVectorA;
						translation = model.translationA;
					}
					else {
						rotationVector = model.system.initialRotationVector;
						translation = model.system.initialTranslation;
					}
				}

				//TODO
				// * check transforms are correct order + inversion
				// * consider using Jacobian
				// * cancel transform from camera A
				if (success) {
					this->dataPreview.transformStereoResult = ofxCv::makeMatrix(rotationVector, translation);
				}

				return success;
			}























#pragma mark StereoSolvePnP Node
			//----------
			StereoSolvePnP::StereoSolvePnP() {
				RULR_NODE_INIT_LISTENER;
			}

			//----------
			std::string StereoSolvePnP::getTypeName() const {
				return "MoCap::StereoSolvePnP";
			}

			//----------
			void StereoSolvePnP::init() {
				RULR_NODE_UPDATE_LISTENER;
				RULR_NODE_DRAW_WORLD_LISTENER;

				this->addInput<Procedure::Calibrate::StereoCalibrate>();
				this->addInput<Item::AbstractBoard>();

				this->manageParameters(this->parameters);

				//setup gui
				{
					auto panel = ofxCvGui::Panels::Groups::makeStrip();
					auto panelA = ofxCvGui::Panels::makeTexture(this->dataPreview.previewB);
					panelA->onDrawImage += [this](ofxCvGui::DrawImageArguments & args) {
						ofxCv::drawCorners(this->dataPreview.imagePointsA);
					};
					auto panelB = ofxCvGui::Panels::makeTexture(this->dataPreview.previewA);
					panelB->onDrawImage += [this](ofxCvGui::DrawImageArguments & args) {
						ofxCv::drawCorners(this->dataPreview.imagePointsB);
					};
					panel->add(panelA);
					panel->add(panelB);
					this->panel = panel;
				}
			}

			//----------
			void StereoSolvePnP::update() {
				try {
					this->throwIfMissingAnyConnection();
					auto stereoCalibrationNode = this->getInput<Procedure::Calibrate::StereoCalibrate>();
					auto boardNode = this->getInput<Item::AbstractBoard>();

					//get the cameras
					stereoCalibrationNode->throwIfACameraIsDisconnected();
					auto cameraNodeA = stereoCalibrationNode->getInput<Item::Camera>("Camera A");
					auto cameraNodeB = stereoCalibrationNode->getInput<Item::Camera>("Camera B");

					//get the frames
					cv::Mat imageA;
					cv::Mat imageB;
					{
						auto frameA = cameraNodeA->getFrame();
						if (!frameA) {
							throw(ofxRulr::Exception("No frame available in Camera A"));
						}
						imageA = ofxCv::toCv(frameA->getPixels());
						this->dataPreview.previewB.loadData(frameA->getPixels());

						auto frameB = cameraNodeB->getFrame();
						if (!frameB) {
							throw(ofxRulr::Exception("No frame available in Camera B"));
						}
						imageB = ofxCv::toCv(frameB->getPixels());
						this->dataPreview.previewA.loadData(frameB->getPixels());
					}

					//find the boards
					vector<cv::Point2f> imagePointsA;
					vector<cv::Point2f> imagePointsB;
					vector<cv::Point3f> objectPointsA;
					vector<cv::Point3f> objectPointsB;
					{
						auto findBoardMode = this->parameters.findBoardMode.get();
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
							this->dataPreview.imagePointsA.clear();
							throw(ofxRulr::Exception("Board not found in Image A"));
						}
						if (!futureB.get()) {
							this->dataPreview.imagePointsB.clear();
							throw(ofxRulr::Exception("Board not found in Image B"));
						}

						this->dataPreview.imagePointsA = imagePointsA;
						this->dataPreview.imagePointsB = imagePointsB;
					}

					cv::Mat rotationVector;
					cv::Mat translation;
					if (solvePnPStereo(stereoCalibrationNode
						, imagePointsA
						, imagePointsB
						, objectPointsA
						, objectPointsB
						, rotationVector
						, translation
						, false)) {
					}
				}
				RULR_CATCH_ALL_TO_ERROR;
			}

			//----------
			ofxCvGui::PanelPtr StereoSolvePnP::getPanel() {
				return this->panel;
			}

			//----------
			void StereoSolvePnP::drawWorld() {
				if (this->parameters.draw.a) {
					ofPushMatrix();
					{
						ofMultMatrix(this->dataPreview.transformA);
						ofDrawAxis(0.1f);
						ofDrawBitmapString("A", 0, 0);
					}
					ofPopMatrix();
				}
				
				if (this->parameters.draw.b) {
					ofPushMatrix();
					{
						ofMultMatrix(this->dataPreview.transformB);
						ofDrawAxis(0.1f);
						ofDrawBitmapString("B", 0, 0);
					}
					ofPopMatrix();
				}

				if (this->parameters.draw.stereo) {
					ofPushMatrix();
					{
						ofMultMatrix(this->dataPreview.transformStereoResult);
						ofDrawAxis(0.1f);
						ofDrawBitmapString("S", 0, 0);
					}
					ofPopMatrix();
				}
			}
		}
	}
}
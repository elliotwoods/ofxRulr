#include "pch_Plugin_MoCap.h"
#include "StereoSolvePnP.h"

#include "ofxRulr/Nodes/Procedure/Calibrate/StereoCalibrate.h"
#include "ofxRulr/Nodes/Item/Camera.h"
#include "ofxRulr/Nodes/Item/AbstractBoard.h"

namespace ofxRulr {
	namespace Nodes {
		namespace MoCap {

			struct SolvePnPDataPoint {
				vector<cv::Point3f> objectSpacePoint;
				vector<cv::Point2f> imagePointProjectionA;
				vector<cv::Point2f> imagePointProjectionB;
			};

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

				double getResidual(SolvePnPDataPoint dataPoint) const override {
					auto dataPointTest = dataPoint;
					this->evaluate(dataPointTest);

					double residualSquaredSum = 0.0;
					for (int i = 0; i < dataPoint.objectSpacePoint.size(); i++) {
						residualSquaredSum += ofxCv::toOf(dataPoint.imagePointProjectionA[i]).squareDistance(ofxCv::toOf(dataPointTest.imagePointProjectionA[i]));
						residualSquaredSum += ofxCv::toOf(dataPoint.imagePointProjectionB[i]).squareDistance(ofxCv::toOf(dataPointTest.imagePointProjectionB[i]));
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
						, this->system.rotationVectorStereoInverse
						, this->system.translationStereoInverse
						, this->rotationVectorB
						, this->translationB);
				}

				System system;

				cv::Mat rotationVectorA;
				cv::Mat rotationVectorB;
				cv::Mat translationA;
				cv::Mat translationB;

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

			//----------
			//hacked from https://github.com/opencv/opencv/blob/3.1.0/modules/calib3d/src/calibration.cpp#L1154
			bool solvePnPStereo(shared_ptr<Procedure::Calibrate::StereoCalibrate> stereoCalibrateNode
				, const vector<cv::Point2f> & imagePointsA
				, const vector<cv::Point2f> & imagePointsB
				, const vector<cv::Point3f> & objectPoints
				, cv::Mat & rotationVector
				, cv::Mat & translation
				, bool useExtrinsicGuess) {

				auto count = imagePointsA.size();
				if (imagePointsB.size() != count
					|| objectPoints.size() != count) {
					throw(ofxRulr::Exception("solvePnP requires vectors with equal length as input"));
				}
				stereoCalibrateNode->throwIfACameraIsDisconnected();
				auto openCVCalibration = stereoCalibrateNode->getOpenCVCalibration();

				//build the model
				SolvePnPModel model;
				{
					auto cameraNodeA = stereoCalibrateNode->getInput<Item::Camera>("Camera A");
					auto cameraNodeB = stereoCalibrateNode->getInput<Item::Camera>("Camera B");

					//get inverse transform between 2 views
					cv::Mat rotationVectorInverse;
					cv::Mat translationInverse;
					{
						auto stereoTransform = ofxCv::makeMatrix(openCVCalibration.rotationVector, openCVCalibration.translation);
						auto stereoTransformInverse = stereoTransform.getInverse();
						ofxCv::decomposeMatrix(stereoTransformInverse, rotationVectorInverse, translationInverse);
					}

					model.system = SolvePnPModel::System{
						cv::Mat()
						, cv::Mat()
						, cameraNodeA->getCameraMatrix()
						, cameraNodeB->getCameraMatrix()
						, cameraNodeA->getDistortionCoefficients()
						, cameraNodeB->getDistortionCoefficients()
						, rotationVectorInverse
						, translationInverse
					};
				}

				//initialise with single camera
				{
					return cv::solvePnP(objectPoints
						, imagePointsA
						, model.system.cameraMatrixA
						, model.system.distortionCoefficientsA
						, model.system.initialRotationVector
						, model.system.initialTranslation);
				}

				//build the dataSet
				vector<SolvePnPDataPoint> dataSet;
				{
					SolvePnPDataPoint dataPoint;
					dataPoint.imagePointProjectionA = imagePointsA;
					dataPoint.imagePointProjectionB = imagePointsB;
					dataPoint.objectSpacePoint = objectPoints;
					dataSet.emplace_back(dataPoint);
				}

				//perform the fit
				double residual = 0.0;
				ofxNonLinearFit::Fit<SolvePnPModel> fit(ofxNonLinearFit::Algorithm(nlopt::LN_BOBYQA, ofxNonLinearFit::Algorithm::Domain::LocalGradientless));
				auto success = fit.optimise(model, &dataSet, &residual);

				//TODO
				// * build a test case (compare to opencv's solvePnP)
				// * objectPoints don't need to be the same for both cameras
				// * check transforms are correct order + inversion
				// * add error from second camera (+check the order + inversion)
				// * consider using Jacobian
				
				//rotationVector = model.rotationVectorA;
				//translation = model.translationA;

				rotationVector = model.system.initialRotationVector;
				translation = model.system.initialTranslation;

				cout << "rotation " << rotationVector << endl;
				cout << "translation " << translation << endl;
				return success;
			}

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
			}

			//----------
			void StereoSolvePnP::update() {
				if (this->isBeingInspected()) {
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

							auto frameB = cameraNodeB->getFrame();
							if (!frameB) {
								throw(ofxRulr::Exception("No frame available in Camera B"));
							}
							imageB = ofxCv::toCv(frameB->getPixels());
						}

						//find the boards
						vector<cv::Point2f> imagePointsA;
						vector<cv::Point2f> imagePointsB;
						vector<cv::Point3f> objectPoints;
						{
							vector<cv::Point3f> objectPointsA;
							vector<cv::Point3f> objectPointsB;

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

							futureA.get();
							futureB.get();

							Item::AbstractBoard::filterCommonPoints(imagePointsA
								, imagePointsB
								, objectPointsA
								, objectPointsB);

							if (imagePointsA.size() < 3) {
								throw(ofxRulr::Exception("Not enough points seen in both cameras"));
							}

							objectPoints = objectPointsA;
						}

						cv::Mat rotationVector;
						cv::Mat translation;
						if (solvePnPStereo(stereoCalibrationNode
							, imagePointsA
							, imagePointsB
							, objectPoints
							, rotationVector
							, translation
							, true)) {
						}
						this->transformA = ofxCv::makeMatrix(rotationVector, translation);

						//get transform B
						{
							auto openCVCalibration = this->getInput<Procedure::Calibrate::StereoCalibrate>()->getOpenCVCalibration();

							cv::Mat rotationVectorB;
							cv::Mat translationB;

							cv::composeRT(rotationVector
								, translation
								, openCVCalibration.rotationVector
								, openCVCalibration.translation
								, rotationVectorB
								, translationB);

							this->transformB = ofxCv::makeMatrix(rotationVectorB, translationB);
						}
					}
					catch (...) {

					}
				}
			}

			//----------
			ofxCvGui::PanelPtr StereoSolvePnP::getPanel() {
				return this->panel;
			}

			//----------
			void StereoSolvePnP::drawWorld() {
				ofPushMatrix();
				{
					ofMultMatrix(this->transformA);
					ofDrawAxis(0.1f);
					ofDrawBitmapString("A", 0, 0);
				}
				ofPopMatrix();

				ofPushMatrix();
				{
					ofMultMatrix(this->transformB);
					ofDrawAxis(0.1f);
					ofDrawBitmapString("B", 0, 0);
				}
				ofPopMatrix();
			}
		}
	}
}
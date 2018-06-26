#include "pch_Plugin_Experiments.h"

namespace ofxRulr {
	namespace Nodes {
		namespace Experiments {
			namespace PhotoScan {
				//----------
				CalibrateProjector::CalibrateProjector() {
					RULR_NODE_INIT_LISTENER;
				}

				//----------
				string CalibrateProjector::getTypeName() const {
					return "Experiments::PhotoScan::CalibrateProjector";
				}

				//----------
				void CalibrateProjector::init() {
					RULR_NODE_INSPECTOR_LISTENER;

					this->addInput<PhotoScan::BundlerCamera>();
					this->addInput<Procedure::Scan::Graycode>();
					this->addInput<Item::Projector>();

					this->panel = ofxCvGui::Panels::makeImage(this->preview.mask, "Mask");

					this->manageParameters(this->parameters);
				}

				//----------
				void CalibrateProjector::calibrate() {
					this->throwIfMissingAnyConnection();

					auto bundlerCameraNode = this->getInput<BundlerCamera>();
					auto graycodeNode = this->getInput<Procedure::Scan::Graycode>();
					auto projectorNode = this->getInput<Item::Projector>();

					//get the bundler capture
					shared_ptr<BundlerCamera::Capture> bundlerCamera;
					{
						auto bundlerCameras = bundlerCameraNode->getSelectedCaptures();
						if (bundlerCameras.size() != 1) {
							throw(ofxRulr::Exception("Please select exactly one bundler capture on its node"));
						}
						bundlerCamera = bundlerCameras.front();
					}

					//get the graycode data
					const auto & graycodeData = graycodeNode->getDataSet();
					{
						if (!graycodeNode->hasData()) {
							throw(ofxRulr::Exception("No graycode data is available"));
						}
					}

					//create a mask of discontinuities in camera space for the scan
					cv::Mat mask;
					{
						cv::Mat projectorInCameraPreview = ofxCv::toCv(graycodeNode->getDecoder().getProjectorInCamera().getPixels());
						cv::Mat sobel;
						cv::Sobel(projectorInCameraPreview
							, sobel
							, CV_16S
							, 1
							, 1);

						cv::Mat absSobel = cv::abs(sobel);
						cv::Mat absSobel8;
						absSobel.convertTo(absSobel8, CV_8U);
						cv::cvtColor(absSobel8, absSobel8, cv::COLOR_RGB2GRAY);

						cv::Mat thresholded;
						cv::threshold(absSobel8
							, thresholded
							, this->parameters.disocontinuityFiltering.mask.threshold
							, 255
							, cv::THRESH_BINARY);

						//dilate the bad regions
						if (this->parameters.disocontinuityFiltering.mask.dilations.get() > 0) {
							cv::dilate(thresholded
								, thresholded
								, cv::Mat()
								, cv::Point(-1, -1)
								, this->parameters.disocontinuityFiltering.mask.dilations);
						}

						//get the active pixels from the scan as the mask
						auto activePixels = graycodeData.getActive();
						mask = ofxCv::toCv(activePixels).clone();

						//remove any large changing areas from the mask
						mask.setTo(0, thresholded);

						//update the preview
						ofxCv::copy(mask, this->preview.mask.getPixels());
						this->preview.mask.update();
					}

					//initialise projector size
					const auto projectorWidth = graycodeData.getPayloadWidth();
					const auto projectorHeight = graycodeData.getPayloadHeight();
					{
						projectorNode->setWidth(projectorWidth);
						projectorNode->setHeight(projectorHeight);

					}
					
					const auto cameraWidth = graycodeData.getWidth();

					//get all the relevant tie points in camera space (filter for mask)
					vector<cv::Point3f> worldPoints;
					vector<cv::Point2f> projectorImagePoints;
					for (int i = 0; i < bundlerCamera->worldPoints.size(); i++) {
						auto & worldPoint = bundlerCamera->worldPoints[i];
						auto & cameraImagePoint = bundlerCamera->imagePoints[i];

						//check it's within the mask
						uint8_t maskValue = mask.data[(int) cameraImagePoint.y * cameraWidth + (int) cameraImagePoint.x];
						if (maskValue == 0) {
							//this is outside of the mask
							continue;
						}

						//get the projector pixel data for this
						auto cameraPixelIndex = graycodeData.getData().getPixelIndex(cameraImagePoint.x, cameraImagePoint.y);
						auto projectorPixelIndex = graycodeData.getData().getData()[cameraPixelIndex];

						worldPoints.push_back(worldPoint);
						projectorImagePoints.push_back(cv::Point2f(projectorPixelIndex % projectorWidth, projectorPixelIndex / projectorWidth));
					}

					//initialise the projector fov
					auto cameraMatrix = projectorNode->getCameraMatrix();
					auto distortionCoefficients = projectorNode->getDistortionCoefficients();
					auto initParams = [&]() {
						auto initialThrowRatio = this->parameters.calibrate.initial.throwRatio;
						auto initialLensOffset = this->parameters.calibrate.initial.lensOffset;

						cameraMatrix = cv::Mat::eye(3, 3, CV_64F);
						cameraMatrix.at<double>(0, 0) = projectorWidth * initialThrowRatio;
						cameraMatrix.at<double>(1, 1) = projectorWidth * initialThrowRatio;
						cameraMatrix.at<double>(0, 2) = projectorWidth / 2.0f;
						cameraMatrix.at<double>(1, 2) = projectorHeight * (0.50f - initialLensOffset / 2.0f);
						distortionCoefficients = cv::Mat::zeros(5, 1, CV_64F);
					};
					initParams();

					vector<cv::Mat> rotationVectors, translations;

					cout << "Calibrating projector with " << worldPoints.size() << " points" << endl;

					auto calib = [&]() {
						this->reprojectionError = cv::calibrateCamera(vector<vector<cv::Point3f>>(1, worldPoints)
							, vector<vector<cv::Point2f>>(1, projectorImagePoints)
							, projectorNode->getSize()
							, cameraMatrix
							, distortionCoefficients
							, rotationVectors
							, translations
							, CV_CALIB_FIX_K1
							| CV_CALIB_FIX_K2
							| CV_CALIB_FIX_K3
							| CV_CALIB_FIX_K4
							| CV_CALIB_FIX_K5
							| CV_CALIB_FIX_K6
							| CV_CALIB_ZERO_TANGENT_DIST
							| CV_CALIB_USE_INTRINSIC_GUESS
							| CV_CALIB_FIX_ASPECT_RATIO);
					};
					calib();

					//check reprojections
					if(this->parameters.calibrate.filterOutliers) {
						vector<cv::Point2f> imagePointsReprojected;
						cv::projectPoints(worldPoints
							, rotationVectors.front()
							, translations.front()
							, cameraMatrix
							, distortionCoefficients
							, imagePointsReprojected);

						set<int> outlierIndicies;
						for (int i = 0; i < imagePointsReprojected.size(); i++) {
							auto delta = ofxCv::toOf(imagePointsReprojected[i]) - ofxCv::toOf(projectorImagePoints[i]);
							auto reprojectionError = delta.length();
							cout << "[" << i << "] " << reprojectionError << endl;

							if (reprojectionError > this->parameters.calibrate.maximumReprojectionError) {
								outlierIndicies.insert(i);
							}
						}

						//trim world points
						{
							auto index = 0;
							for (auto it = worldPoints.begin(); it != worldPoints.end(); ) {
								if (outlierIndicies.find(index++) != outlierIndicies.end()) {
									it = worldPoints.erase(it);
								}
								else {
									it++;
								}
							}
						}

						//trim image points
						{
							auto index = 0;
							for (auto it = projectorImagePoints.begin(); it != projectorImagePoints.end(); ) {
								if (outlierIndicies.find(index++) != outlierIndicies.end()) {
									it = projectorImagePoints.erase(it);
								}
								else {
									it++;
								}
							}
						}

						//calib again with trimmed points
						initParams();
						calib();
					}

					projectorNode->setIntrinsics(cameraMatrix
						, distortionCoefficients);
					projectorNode->setExtrinsics(rotationVectors.front()
						, translations.front()
						, true);
				}

				//----------
				ofxCvGui::PanelPtr CalibrateProjector::getPanel() {
					return this->panel;
				}

				//----------
				void CalibrateProjector::populateInspector(ofxCvGui::InspectArguments & args) {
					auto inspector = args.inspector;
					inspector->addButton("Calibrate", [this]() {
						try {
							Utils::ScopedProcess scopedProcess("Calibrating");
							this->calibrate();
							scopedProcess.end();
						}
						RULR_CATCH_ALL_TO_ALERT;
					}, OF_KEY_RETURN)->setHeight(100.0f);
					inspector->addLiveValue(this->reprojectionError);
				}

				//----------
				void CalibrateProjector::drawWorldStage() {

				}
			}
		}
	}
}
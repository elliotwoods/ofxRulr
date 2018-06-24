#include "pch_Plugin_Experiments.h"
#include "BundlerCamera.h"

namespace ofxRulr {
	namespace Nodes {
		namespace Experiments {
			namespace PhotoScan {
				//----------
				void BundlerCamera::Capture::drawWorld() {
					this->preview.drawVertices();
				}

				//----------
				string BundlerCamera::Capture::getDisplayString() const {
					stringstream ss;
					ss << "Tie points : " << this->worldPoints.size();
					return ss.str();
				}

				//----------
				BundlerCamera::BundlerCamera() {
					RULR_NODE_INIT_LISTENER;
				}

				//----------
				string BundlerCamera::getTypeName() const {
					return "Experiments::PhotoScan::BundlerCamera";
				}

				//----------
				void BundlerCamera::init() {
					RULR_NODE_INSPECTOR_LISTENER;
					RULR_NODE_DRAW_WORLD_LISTENER;

					this->addInput<Item::Camera>();

					this->previewTilePoints.allocate(1, 1, GL_RGBA);
					this->panel = ofxCvGui::Panels::makeTexture(this->previewTilePoints.getTexture());

					this->manageParameters(this->parameters);
				}

				//----------
				void BundlerCamera::addCapture() {

				}

				//----------
				void BundlerCamera::calibrate() {
					this->throwIfMissingAnyConnection();
					auto camera = this->getInput<Item::Camera>();

					auto captures = this->captures.getSelection();
					if (captures.size() != 1) {
						throw(ofxRulr::Exception("Please select exactly one capture when calibrating camera"));
					}

					auto selectedCapture = captures[0];

					vector<vector<cv::Point3f>> worldPointsSets;
					vector<vector<cv::Point2f>> imagePointsSets;
					{
						const auto cameraWidth = camera->getWidth();
						const auto cameraHeight = camera->getHeight();

						//function to add a capture to the dataset
						auto addToSet = [this, cameraWidth, cameraHeight, &worldPointsSets, &imagePointsSets](shared_ptr<Capture> capture, bool drawOnPreview) {
							vector<cv::Point3f> worldPoints;
							vector<cv::Point2f> imagePoints;

							for (int i = 0; i < capture->imagePoints.size(); i += this->parameters.calibrate.decimator) {
								const auto & rawWorldPoint = capture->worldPoints[i];
								const auto & rawImagePoint = capture->imagePoints[i];

								auto imagePoint = cv::Point2f(cameraWidth / 2.0f + rawImagePoint.x
									, cameraHeight / 2.0f - rawImagePoint.y
								);

								worldPoints.push_back(rawWorldPoint);
								imagePoints.push_back(imagePoint);

								if (drawOnPreview) {
									ofPushStyle();
									{
										ofSetColor(capture->tiePointColors[i]);
										ofDrawCircle(ofxCv::toOf(imagePoint), this->parameters.preview.dotSize);
									}
									ofPopStyle();
								}
							}

							worldPointsSets.push_back(worldPoints);
							imagePointsSets.push_back(imagePoints);
						};

						//transform the points and draw to preview as we go along
						this->previewTilePoints.allocate(cameraWidth, cameraHeight, GL_RGBA);
						this->previewTilePoints.begin();
						{
							ofClear(0, 255);
							addToSet(selectedCapture, true);
						}
						this->previewTilePoints.end();

						//add all the other captures if we want them
						if (this->parameters.calibrate.useAllForCalib) {
							auto allCaptures = this->captures.getAllCaptures();
							for (auto capture : allCaptures) {
								if (capture == selectedCapture) {
									//don't add it twice
									continue;
								}
								addToSet(capture, false);
							}
						}
					}

					cv::Mat cameraMatrix = camera->getCameraMatrix();
					cv::Mat distortionCoefficients = camera->getDistortionCoefficients();
					vector<cv::Mat> rotationVectors, translations;

					if (this->parameters.calibrate.initialiseWithBundlerCalibration) {
						cameraMatrix.at<double>(0, 0) = selectedCapture->f;
						cameraMatrix.at<double>(1, 1) = selectedCapture->f;
						cameraMatrix.at<double>(0, 2) = camera->getWidth() / 2.0f;
						cameraMatrix.at<double>(1, 2) = camera->getHeight() / 2.0f;
					}

					this->reprojectionError = cv::calibrateCamera(worldPointsSets
						, imagePointsSets
						, camera->getSize()
						, cameraMatrix
						, distortionCoefficients
						, rotationVectors
						, translations
						, CV_CALIB_USE_INTRINSIC_GUESS | CV_CALIB_FIX_K5 | CV_CALIB_FIX_K6);

					camera->setIntrinsics(cameraMatrix, distortionCoefficients);
					camera->setExtrinsics(rotationVectors[0]
						, translations[0]
						, true);
				}

				//----------
				ofxCvGui::PanelPtr BundlerCamera::getPanel() {
					return this->panel;
				}

				//----------
				void BundlerCamera::populateInspector(ofxCvGui::InspectArguments & args) {
					auto inspector = args.inspector;

					inspector->addButton("Import bundler...", [this]() {
						try {
							Utils::ScopedProcess scopedProcess("Loading bundler file");

							auto fileResult = ofSystemLoadDialog("Select bundler_formatted.json file");
							if (!fileResult.bSuccess) {
								return;
							}

							this->importBundler(fileResult.filePath);
							scopedProcess.end();
						}
						RULR_CATCH_ALL_TO_ALERT;
					});

					this->captures.populateWidgets(inspector);

					inspector->addButton("Calibrate", [this]() {
						try {
							Utils::ScopedProcess scopedProcess("Calibrating...");
							this->calibrate();
							scopedProcess.end();
						}
						RULR_CATCH_ALL_TO_ALERT;
					}, OF_KEY_RETURN)->setHeight(100.0f);

					inspector->addLiveValue(this->reprojectionError);
				}

				//----------
				void BundlerCamera::drawWorldStage() {
					auto captures = this->captures.getSelection();
					for (auto capture : captures) {
						capture->drawWorld();
					}
				}

				//----------
				void BundlerCamera::importBundler(const string & filePath) {
					auto fileContents = ofFile(filePath).readToBuffer();
					Json::Value json;
					Json::Reader().parse((const string &)fileContents, json);

					for (const auto & jsonView : json) {
						auto capture = make_shared<Capture>();
						capture->f = jsonView["f"].asFloat();
						capture->k1 = jsonView["k1"].asFloat();
						capture->k2 = jsonView["k2"].asFloat();

						const auto & jsonTiePoints = jsonView["tiePoints"];
						for (const auto & jsonTiePoint : jsonTiePoints) {
							ofVec3f worldPoint;
							for (int i = 0; i < 3; i++) {
								worldPoint[i] = jsonTiePoint["worldSpace"][i].asFloat();
							}

							ofVec2f imagePoint;
							for (int i = 0; i < 2; i++) {
								imagePoint[i] = jsonTiePoint["imageSpace"][i].asFloat();
							}

							ofFloatColor color(255, 255, 255, 255);
							for (int i = 0; i < 3; i++) {
								color[i] = jsonTiePoint["color"][i].asFloat();
							}
							color /= 255;

							capture->worldPoints.push_back(ofxCv::toCv(worldPoint));
							capture->imagePoints.push_back(ofxCv::toCv(imagePoint));
							capture->tiePointColors.push_back(color);
						}

						capture->preview.addVertices(ofxCv::toOf(capture->worldPoints));
						capture->preview.addColors(capture->tiePointColors);

						this->captures.add(capture);
					}
				}
			}
		}
	}
}

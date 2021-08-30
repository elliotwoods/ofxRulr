#include "pch_Plugin_ArUco.h"

#include "Detector.h"
#include "ofxRulr/Nodes/GraphicsManager.h"

struct MarkerDetectorClone {
	aruco::MarkerDetector markerDetector;
	aruco::Dictionary dictionary;
	aruco::Dictionary::DICT_TYPES dictionaryType;
};

namespace ofxRulr {
	namespace Nodes {
		namespace ArUco {
			//----------
			Detector::Detector() {
				RULR_NODE_INIT_LISTENER;
				this->setIcon(Nodes::GraphicsManager::X().getIcon("ArUco::Base"));
			}

			//----------
			std::string Detector::getTypeName() const {
				return "ArUco::Detector";
			}

			//----------
			void Detector::init() {
				RULR_NODE_UPDATE_LISTENER;
				RULR_NODE_INSPECTOR_LISTENER;
				RULR_NODE_SERIALIZATION_LISTENERS;

				this->rebuildDetector();
				
				//set the default
				this->parameters.dictionary = DetectorType::MIP_3612h;

				{
					auto panel = ofxCvGui::Panels::makeImage(this->preview);
					panel->onDrawImage += [this](ofxCvGui::DrawImageArguments & args) {
						auto previewType = this->parameters.debug.preview.get();
						if (previewType == Preview::Raw
							|| previewType == Preview::Normalised) {
							ofPushStyle();
							{
								ofNoFill();
								ofSetColor(255, 100, 100);
								ofSetLineWidth(2.0f);

								for (const auto & marker : this->foundMarkers) {
									ofPolyline line;
									for (const cv::Point2f & point : marker) {
										line.addVertex({
											point.x
											, point.y
											, 0.0f
											});
									}
									line.close();
									line.draw();

									ofDrawBitmapString(ofToString(marker.id), ofxCv::toOf(marker.getCenter()));
								}
							}
							ofPopStyle();
						}
					};
					this->panel = panel;
				}

				//add listeners
				this->parameters.dictionary.addListener(this, &Detector::changeDetectorCallback);
				this->parameters.arucoDetector.threads.addListener(this, &Detector::changeIntCallback);
				this->parameters.arucoDetector.enclosedMarkers.addListener(this, &Detector::changeBoolCallback);
				this->parameters.arucoDetector.thresholdAttempts.addListener(this, &Detector::changeIntCallback);
				this->parameters.arucoDetector.adaptiveThreshold.windowSize.addListener(this, &Detector::changeIntCallback);
				this->parameters.arucoDetector.adaptiveThreshold.windowSizeRange.addListener(this, &Detector::changeIntCallback);
				this->parameters.arucoDetector.threshold.addListener(this, &Detector::changeIntCallback);
			}

			//----------
			void Detector::update() {
				if(this->detectorDirty) {
					this->rebuildDetector();
				}
				if (this->cachedPreviewType != this->parameters.debug.preview.get()) {
					switch (this->parameters.debug.preview.get()) {
					case Preview::Raw:
						ofxCv::copy(lastDetection.rawImage, this->preview);
						break;
					case Preview::Normalised:
						ofxCv::copy(lastDetection.normalisedImage, this->preview);
						break;
					case Preview::Thresholded:
					{
						auto& thresholdedImage = this->markerDetector.getThresholdedImage();
						if (!thresholdedImage.empty()) {
							ofxCv::copy(lastDetection.thresholded, this->preview);
						}
					}
					break;
					case Preview::None:
						this->preview.clear();
					default:
						break;
					}
					this->preview.update();
				}
			}

			//----------
			void Detector::populateInspector(ofxCvGui::InspectArguments & inspectArgs) {
				auto inspector = inspectArgs.inspector;
				inspector->addParameterGroup(this->parameters);
				inspector->addLiveValue<string>("Dictionary type", [this]() {
					return aruco::Dictionary::getTypeString(this->dictionaryType);
				});
				inspector->addButton("Retry detect", [this]() {
					Utils::ScopedProcess scopedProcess("Retry detection...", false);
					this->findMarkers(this->lastDetection.rawImage, false);
					cout << this->foundMarkers.size() << " markers found." << endl;
				}, ' ');
			}

			//----------
			void Detector::serialize(nlohmann::json & json) {
				Utils::serialize(json, this->parameters);
			}

			//----------
			void Detector::deserialize(const nlohmann::json & json) {
				Utils::deserialize(json, this->parameters);
			}

			//----------
			aruco::MarkerDetector & Detector::getMarkerDetector() {
				return this->markerDetector;
			}

			//----------
			aruco::Dictionary & Detector::getDictionary() {
				return this->dictionary;
			}

			//----------
			const ofImage & Detector::getMarkerImage(int markerIndex) {
				auto findImage = this->cachedMarkerImages.find(markerIndex);
				if (findImage == this->cachedMarkerImages.end()) {
					auto image = make_shared<ofImage>();
					auto mat = this->dictionary.getMarkerImage_id(markerIndex, 8, false);
					ofxCv::copy(mat, image->getPixels());
					image->getPixels().resize(ARUCO_PREVIEW_RESOLUTION, ARUCO_PREVIEW_RESOLUTION, OF_INTERPOLATE_NEAREST_NEIGHBOR);
					image->update();
					image->getTexture().setTextureMinMagFilter(GL_NEAREST, GL_NEAREST);

					findImage = this->cachedMarkerImages.emplace(markerIndex, move(image)).first;
				}
				return *findImage->second;
			}

			//----------
			float Detector::getMarkerLength() const {
				return this->parameters.markerLength;
			}

			//----------
			ofxCvGui::PanelPtr Detector::getPanel() {
				return this->panel;
			}

			//----------
			std::vector<aruco::Marker>Detector::findMarkers(const cv::Mat & image, bool fromAnotherThread) {
				Frame frame;

				frame.rawImage = image.clone();

				if (frame.rawImage.channels() == 3) {
					cv::cvtColor(frame.rawImage
						, frame.rawImage
						, cv::COLOR_RGB2GRAY);
				}

				// Merge function
				vector<aruco::Marker> foundMarkers;

				std::mutex lockFoundMarkers;
				auto mergeResults = [&](const vector<aruco::Marker>& newMarkers) {
					lockFoundMarkers.lock();
					{
						// Add data to result
						for (const auto& newMarker : newMarkers) {
							bool foundInPriorData = false;
							for (const auto& priorMarker : foundMarkers) {
								if (priorMarker.id == newMarker.id) {
									foundInPriorData = true;
									break;
								}
							}
							if (!foundInPriorData) {
								foundMarkers.push_back(newMarker);
							}
						}
					}
					lockFoundMarkers.unlock();
				};

				// Detector clone function
				auto getDetecterClone = [&]() {
					auto markerDetectorClone = make_shared<MarkerDetectorClone>();
					stringstream ss;

					this->markerDetector.toStream(ss);
					markerDetectorClone->markerDetector.fromStream(ss);
					this->buildDetector(markerDetectorClone->markerDetector
						, markerDetectorClone->dictionary
						, markerDetectorClone->dictionaryType);

					return markerDetectorClone;
				};

				// Add strategy
				vector<future<void>> strategies;
				auto addStrategy = [&](const function<void()>& strategy) {
					strategies.push_back(async(launch::async, strategy));
				};

				// Strategies
				{
					// Direct find
					addStrategy([&]() {
						auto detectorClone = getDetecterClone();
						auto directMarkersFound = detectorClone->markerDetector.detect(frame.rawImage);
						mergeResults(directMarkersFound);
						});

					// Normalize
					if (this->parameters.strategies.normalize.enabled.get()) {
						frame.rawImage.convertTo(frame.normalisedImage, CV_32F);

						// Push values up so that mid value is middle of range 0-255
						auto midValue = cv::mean(frame.normalisedImage)[0];
						if (midValue < 127) {
							frame.normalisedImage *= 127.0f / midValue;
						}

						// Copy back to 8 bit
						frame.normalisedImage.convertTo(frame.normalisedImage
							, CV_8U);

						// Normalise
						cv::normalize(frame.normalisedImage
							, frame.normalisedImage
							, 255
							, 0
							, cv::NormTypes::NORM_MINMAX);

						addStrategy([&]() {
							auto markerDetectorClone = getDetecterClone();
							auto newMarkersFound = markerDetectorClone->markerDetector.detect(frame.normalisedImage);
							mergeResults(newMarkersFound);
						});
					}
					else {
						frame.normalisedImage = frame.rawImage;
					}

					// Multi-crop
					if (this->parameters.strategies.multiCrop.enabled.get()) {
						auto imageWidth = image.cols;
						auto imageHeight = image.rows;

						map<int, aruco::Marker> markersInAllCrops;

						auto iterations = this->parameters.strategies.multiCrop.iterations.get();
						auto overlap = this->parameters.strategies.multiCrop.overlap.get();

						for (int cropIteration = 0; cropIteration < iterations; cropIteration++) {
							auto stepRatio = 1.0f / pow(2, cropIteration);
							int stepWidth = imageWidth * stepRatio;
							int stepHeight = imageHeight * stepRatio;

							if (stepWidth == 0 || stepHeight == 0) {
								continue;
							}

							int overlapWidth = stepWidth * overlap;
							int overlapHeight = stepHeight * overlap;

							for (int x = 0; x <= imageWidth - stepWidth; x += stepWidth) {
								for (int y = 0; y <= imageHeight - stepHeight; y += stepHeight) {
									//calc clamped window within image
									int x_clamped = max(x - overlapWidth, 0);
									int y_clamped = max(y - overlapHeight, 0);

									int width_clamped = stepWidth + overlapWidth;
									int height_clamped = stepHeight + overlapHeight;

									width_clamped = min(width_clamped, imageWidth - x);
									height_clamped = min(height_clamped, imageHeight - y);

									// Check we have an image
									if (width_clamped == 0 || height_clamped == 0) {
										continue;
									}

									// Perform the find
									addStrategy([=]() {
										auto markerDetectorClone = getDetecterClone();

										cv::Rect roi(x_clamped, y_clamped, width_clamped, height_clamped);
										cv::Mat cropped = image(roi);

										//perform detection
										auto markersInCrop = markerDetectorClone->markerDetector.detect(cropped);

										//translate into original image coords
										for (auto& markerInCrop : markersInCrop) {
											for (auto& point : markerInCrop) {
												point.x += roi.x;
												point.y += roi.y;
											}
										}

										mergeResults(markersInCrop);
										});
								}
							}
						}
					}

					// Multi-brightess
					if (this->parameters.strategies.multiBrightness.enabled.get()) {
						for (int i = 2; i < this->parameters.strategies.multiBrightness.maxBrightess.get(); i++) {
							addStrategy([&]() {
								cv::Mat brighterImage;
								frame.rawImage.convertTo(brighterImage
									, CV_8U
									, i
									, 0);
								auto markerDetectorClone = getDetecterClone();
								auto markersFoundInBrightenedImage = markerDetectorClone->markerDetector.detect(brighterImage);
								mergeResults(markersFoundInBrightenedImage);
							});
						}
					}
				}


				for (auto& future : strategies) {
					future.wait();
				}

				// refine corners 1
				{
					auto findRatio = this->parameters.cornerRefinement.zone1.get();
					if (findRatio > 0.0f) {
						for (auto& marker : foundMarkers) {
							auto length2 = 0.0f;
							length2 += glm::length2(ofxCv::toOf(marker[1] - marker[0]));
							length2 += glm::length2(ofxCv::toOf(marker[2] - marker[1]));
							length2 += glm::length2(ofxCv::toOf(marker[3] - marker[2]));
							length2 += glm::length2(ofxCv::toOf(marker[0] - marker[3]));
							length2 /= 4.0f;
							auto searchLength = sqrt(length2) * findRatio;
							
							auto windowSize = (int)searchLength;
							windowSize = ((windowSize / 2) * 2) + 1;

							cv::cornerSubPix(frame.rawImage
								, (vector<cv::Point2f>&) marker
								, cv::Size(windowSize, windowSize)
								, cv::Size(1, 1)
								, cv::TermCriteria(cv::TermCriteria::MAX_ITER + cv::TermCriteria::EPS, 100, 1e-6));
						}
					}
				}

				// refine corners 2 (same code, different settings)
				{
					auto findRatio = this->parameters.cornerRefinement.zone2.get();
					if (findRatio > 0.0f) {
						for (auto& marker : foundMarkers) {
							auto length2 = 0.0f;
							length2 += glm::length2(ofxCv::toOf(marker[1] - marker[0]));
							length2 += glm::length2(ofxCv::toOf(marker[2] - marker[1]));
							length2 += glm::length2(ofxCv::toOf(marker[3] - marker[2]));
							length2 += glm::length2(ofxCv::toOf(marker[0] - marker[3]));
							length2 /= 4.0f;
							auto searchLength = sqrt(length2) * findRatio;

							auto windowSize = (int)searchLength;
							windowSize = ((windowSize / 2) * 2) + 1;

							cv::cornerSubPix(frame.rawImage
								, (vector<cv::Point2f>&) marker
								, cv::Size(windowSize, windowSize)
								, cv::Size(1, 1)
								, cv::TermCriteria(cv::TermCriteria::MAX_ITER + cv::TermCriteria::EPS, 100, 1e-6));
						}
					}
				}

				//speak the count
				if (this->parameters.debug.speakCount && !fromAnotherThread) {
					ofxRulr::Utils::speakCount(foundMarkers.size());
				}

				// Store if on main thread and build preview
				if(!fromAnotherThread) {
					this->lastDetection = frame;
					this->foundMarkers = foundMarkers;
					this->cachedPreviewType = Preview::None;
					this->preview.clear();
					return foundMarkers;
				}
				
				return foundMarkers;
			}

			//----------
			void Detector::rebuildDetector() {
				this->buildDetector(this->markerDetector, this->dictionary, this->dictionaryType);
				this->cachedMarkerImages.clear();
				this->detectorDirty = false;
			}

			//----------
			void Detector::buildDetector(aruco::MarkerDetector& markerDetector, aruco::Dictionary& dictionary, aruco::Dictionary::DICT_TYPES& dictionaryType) const {
				switch (this->parameters.dictionary.get())
				{
				case DetectorType::Original:
					dictionaryType = aruco::Dictionary::ARUCO;
					break;
				case DetectorType::MIP_3612h:
					dictionaryType = aruco::Dictionary::ARUCO_MIP_36h12;
					break;
				case DetectorType::ARTKP:
					dictionaryType = aruco::Dictionary::ARTOOLKITPLUS;
					break;
				case DetectorType::ARTAG:
					dictionaryType = aruco::Dictionary::ARTAG;
					break;
				default:
					throw(ofxRulr::Exception("Detector type not supported"));
					break;
				}

				dictionary = aruco::Dictionary::loadPredefined(this->dictionaryType);
				markerDetector.setDictionary(this->dictionaryType);

				auto& parameters = markerDetector.getParameters();
				parameters.maxThreads = this->parameters.arucoDetector.threads.get();
				parameters.enclosedMarker = this->parameters.arucoDetector.enclosedMarkers.get();
				parameters.NAttemptsAutoThresFix = this->parameters.arucoDetector.thresholdAttempts.get();
				parameters.AdaptiveThresWindowSize = this->parameters.arucoDetector.adaptiveThreshold.windowSize.get();
				parameters.AdaptiveThresWindowSize_range = this->parameters.arucoDetector.adaptiveThreshold.windowSizeRange.get();
				parameters.ThresHold = this->parameters.arucoDetector.threshold.get();
			}

			//----------
			void Detector::changeDetectorCallback(DetectorType &) {
				this->detectorDirty = true;
			}

			//----------
			void Detector::changeFloatCallback(float &) {
				this->detectorDirty = true;
			}

			//----------
			void Detector::changeIntCallback(int&) {
				this->detectorDirty = true;
			}

			//----------
			void Detector::changeBoolCallback(bool&) {
				this->detectorDirty = true;
			}
		}
	}
}
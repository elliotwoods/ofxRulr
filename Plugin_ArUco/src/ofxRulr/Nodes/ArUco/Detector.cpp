#include "pch_Plugin_ArUco.h"

#include "Detector.h"
#include "ofxRulr/Nodes/GraphicsManager.h"


vector<aruco::Marker> multiCropDetect(aruco::MarkerDetector& markerDetector, const cv::Mat& image, int cropIterations, float overlap) {
	auto imageWidth = image.cols;
	auto imageHeight = image.rows;

	map<int, aruco::Marker> markersInAllCrops;

	for (int cropIteration = 0; cropIteration < cropIterations; cropIteration++) {
		auto stepRatio = 1.0f / pow(2, cropIteration);
#ifdef ARUCO_MULTICROP_DEBUG
		cout << cropIteration << ", " << stepRatio << endl;
#endif

		int stepWidth = imageWidth * stepRatio;
		int stepHeight = imageHeight * stepRatio;

		if (stepWidth == 0 || stepHeight == 0) {
#ifdef ARUCO_MULTICROP_DEBUG
			cout << "Skipping crop tier" << endl;
#endif
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

				//check we have an image
				if (width_clamped == 0 || height_clamped == 0) {
#ifdef ARUCO_MULTICROP_DEBUG
					cout << "Skipping crop section" << endl;
#endif
					continue;
				}

				cv::Rect roi(x_clamped, y_clamped, width_clamped, height_clamped);
#ifdef ARUCO_MULTICROP_DEBUG
				cout << roi << "[" << imageWidth << "x" << imageHeight << "]" << endl;
#endif

				cv::Mat cropped = image(roi);

				//perform detection
				auto markersInCrop = markerDetector.detect(cropped);

#ifdef ARUCO_MULTICROP_DEBUG
				//preview result
				cv::imshow("cropped", drawMarkers(cropped, markersInCrop));
				cv::waitKey(0);
#endif

				//translate into image coords
				for (auto& markerInCrop : markersInCrop) {
					for (auto& point : markerInCrop) {
						point.x += roi.x;
						point.y += roi.y;
					}
				}

				//save into markers
				for (const auto& marker : markersInCrop) {
					//this overwrites any previous finds
					markersInAllCrops[marker.id] = marker;
				}
			}
		}
	}

	//assemble vector of markers
	vector<aruco::Marker> markerVector;
	for (const auto& markerIt : markersInAllCrops) {
		markerVector.push_back(markerIt.second);
	}

#ifdef ARUCO_MULTICROP_DEBUG
	//subpix per marker
	for (auto& marker : markerVector) {
		cout << marker.id << ", ";
		auto delta = marker[1] - marker[0];
		auto length2 = delta.dot(delta);
		auto length = sqrt(length2);
		cout << " delta=" << delta;
		cout << " length=" << length;

		auto lengthPart = length / 8; // a square
		lengthPart /= 1; // portion of square

		lengthPart /= 2;
		lengthPart += 1;

		auto winSize = cv::Size(lengthPart, lengthPart);
		cout << "winSize" << winSize << endl;

		//perform subpix
		cv::cornerSubPix(image
			, marker
			, winSize
			, cv::Size(-1, -1)
			, cv::TermCriteria(cv::TermCriteria::MAX_ITER + cv::TermCriteria::EPS, 50, 1e-6));
	}
#endif

	return markerVector;
}

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
					this->findMarkers(this->lastDetection.rawImage);
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
			const std::vector<aruco::Marker> & Detector::findMarkers(const cv::Mat & image) {
				this->lastDetection.rawImage = image.clone();

				if (this->lastDetection.rawImage.channels() == 3) {
					cv::cvtColor(this->lastDetection.rawImage
						, this->lastDetection.rawImage
						, cv::COLOR_RGB2GRAY);
				}

				auto mergeResults = [&](const vector<aruco::Marker>& newMarkers) {
					// Add data to result
					for (const auto& newMarker : newMarkers) {
						bool foundInPriorData = false;
						for (const auto& priorMarker : this->foundMarkers) {
							if (priorMarker.id == newMarker.id) {
								foundInPriorData = true;
								break;
							}
						}
						if (!foundInPriorData) {
							this->foundMarkers.push_back(newMarker);
						}
					}
				};

				// Direct find
				this->foundMarkers = this->markerDetector.detect(this->lastDetection.rawImage);

				// Strategies
				{
					// Normalize
					if (this->parameters.strategies.normalize.enabled.get()) {
						this->lastDetection.rawImage.convertTo(this->lastDetection.normalisedImage, CV_32F);

						// Push values up so that mid value is middle of range 0-255
						auto midValue = cv::mean(this->lastDetection.normalisedImage)[0];
						if (midValue < 127) {
							this->lastDetection.normalisedImage *= 127.0f / midValue;
						}

						// Copy back to 8 bit
						this->lastDetection.normalisedImage.convertTo(this->lastDetection.normalisedImage
							, CV_8U);

						// Normalise
						cv::normalize(this->lastDetection.normalisedImage
							, this->lastDetection.normalisedImage
							, 255
							, 0
							, cv::NormTypes::NORM_MINMAX);

						auto newMarkersFound = this->markerDetector.detect(this->lastDetection.normalisedImage);
						mergeResults(newMarkersFound);
					}
					else {
						this->lastDetection.normalisedImage = this->lastDetection.rawImage;
					}

					// Multi-crop
					if (this->parameters.strategies.multiCrop.enabled.get()) {
						// Find markers in crop (note that if marker is too big for crop strategy then above method works better)
						auto foundMarkersInCrops = multiCropDetect(this->markerDetector
							, this->lastDetection.normalisedImage
							, this->parameters.strategies.multiCrop.iterations.get()
							, this->parameters.strategies.multiCrop.overlap.get());

						mergeResults(foundMarkersInCrops);
					}

					// Multi-brightess
					if (this->parameters.strategies.multiBrightness.enabled.get()) {
						for (int i = 2; i < this->parameters.strategies.multiBrightness.maxBrightess.get(); i++) {
							cv::Mat brighterImage;
							this->lastDetection.rawImage.convertTo(brighterImage
								, CV_8U
								, i
								, 0);
							auto markersFoundInBrightenedImage = this->markerDetector.detect(brighterImage);
							mergeResults(markersFoundInBrightenedImage);
						}
					}
				}

				// refine corners 1
				{
					auto findRatio = this->parameters.cornerRefinement.zone1.get();
					if (findRatio > 0.0f) {
						for (auto& marker : this->foundMarkers) {
							auto length2 = 0.0f;
							length2 += glm::length2(ofxCv::toOf(marker[1] - marker[0]));
							length2 += glm::length2(ofxCv::toOf(marker[2] - marker[1]));
							length2 += glm::length2(ofxCv::toOf(marker[3] - marker[2]));
							length2 += glm::length2(ofxCv::toOf(marker[0] - marker[3]));
							length2 /= 4.0f;
							auto searchLength = sqrt(length2) * findRatio;
							
							auto windowSize = (int)searchLength;
							windowSize = ((windowSize / 2) * 2) + 1;

							cv::cornerSubPix(this->lastDetection.rawImage
								, (vector<cv::Point2f>&) marker
								, cv::Size(windowSize, windowSize)
								, cv::Size(1, 1)
								, cv::TermCriteria(cv::TermCriteria::MAX_ITER + cv::TermCriteria::EPS, 100, 1e-6));
						}
					}
				}

				// refine corners 2 (sane code, different variable)
				{
					auto findRatio = this->parameters.cornerRefinement.zone2.get();
					if (findRatio > 0.0f) {
						for (auto& marker : this->foundMarkers) {
							auto length2 = 0.0f;
							length2 += glm::length2(ofxCv::toOf(marker[1] - marker[0]));
							length2 += glm::length2(ofxCv::toOf(marker[2] - marker[1]));
							length2 += glm::length2(ofxCv::toOf(marker[3] - marker[2]));
							length2 += glm::length2(ofxCv::toOf(marker[0] - marker[3]));
							length2 /= 4.0f;
							auto searchLength = sqrt(length2) * findRatio;

							auto windowSize = (int)searchLength;
							windowSize = ((windowSize / 2) * 2) + 1;

							cv::cornerSubPix(this->lastDetection.rawImage
								, (vector<cv::Point2f>&) marker
								, cv::Size(windowSize, windowSize)
								, cv::Size(1, 1)
								, cv::TermCriteria(cv::TermCriteria::MAX_ITER + cv::TermCriteria::EPS, 100, 1e-6));
						}
					}
				}

				//speak the count
				if (this->parameters.debug.speakCount) {
					ofxRulr::Utils::speakCount(this->foundMarkers.size());
				}

				// build preview
				{
					this->cachedPreviewType = Preview::None;
					this->preview.clear();
				}
				
				return this->foundMarkers;
			}

			//----------
			void Detector::rebuildDetector() {
				switch (this->parameters.dictionary.get())
				{
				case DetectorType::Original:
					this->dictionaryType = aruco::Dictionary::ARUCO;
					break;
				case DetectorType::MIP_3612h:
					this->dictionaryType = aruco::Dictionary::ARUCO_MIP_36h12;
					break;
				case DetectorType::ARTKP:
					this->dictionaryType = aruco::Dictionary::ARTOOLKITPLUS;
					break;
				case DetectorType::ARTAG:
					this->dictionaryType = aruco::Dictionary::ARTAG;
					break;
				default:
					throw(ofxRulr::Exception("Detector type not supported"));
					break;
				}

				this->dictionary = aruco::Dictionary::loadPredefined(this->dictionaryType);
				this->markerDetector.setDictionary(this->dictionaryType);

				auto & parameters = this->markerDetector.getParameters();
				parameters.maxThreads = this->parameters.arucoDetector.threads.get();
				parameters.enclosedMarker = this->parameters.arucoDetector.enclosedMarkers.get();
				parameters.NAttemptsAutoThresFix = this->parameters.arucoDetector.thresholdAttempts.get();
				parameters.AdaptiveThresWindowSize = this->parameters.arucoDetector.adaptiveThreshold.windowSize.get();
				parameters.AdaptiveThresWindowSize_range = this->parameters.arucoDetector.adaptiveThreshold.windowSizeRange.get();
				parameters.ThresHold = this->parameters.arucoDetector.threshold.get();

				this->cachedMarkerImages.clear();
				this->detectorDirty = false;
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
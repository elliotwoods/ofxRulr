#include "pch_Plugin_ArUco.h"

namespace ofxRulr {
	namespace Nodes {
		namespace MarkerMap {
			//----------
			NavigateCamera::NavigateCamera() {
				RULR_NODE_INIT_LISTENER;
			}

			//----------
			string NavigateCamera::getTypeName() const {
				return "MarkerMap::NavigateCamera";
			}

			//----------
			void NavigateCamera::init() {
				RULR_NODE_INSPECTOR_LISTENER;
				RULR_NODE_DRAW_WORLD_LISTENER;

				this->manageParameters(this->parameters);

				this->addInput<Item::Camera>();
				this->addInput<Markers>();
			}

			//----------
			void NavigateCamera::update() {
				if(this->parameters.onNewFrame.get()) {
					auto camera = this->getInput<Item::Camera>();
					if (camera) {
						if (camera->getGrabber()->isFrameNew()) {
							auto frame = camera->getGrabber()->getFrame();
							auto & pixels = frame->getPixels();
							if (pixels.isAllocated()) {
								auto image = ofxCv::toCv(pixels);
								this->track(image);
							}
						}
					}
				}
			}

			//----------
			void NavigateCamera::drawWorldStage() {
				for (const auto& cameraRay : this->cameraRays) {
					cameraRay.draw();
				}
			}

			//----------
			void NavigateCamera::track(const cv::Mat& image) {
				this->throwIfMissingAnyConnection();

				auto markersNode = this->getInput<Markers>();
				auto camera = this->getInput<Item::Camera>();

				markersNode->throwIfMissingAConnection<ArUco::Detector>();
				auto detector = markersNode->getInput<ArUco::Detector>();

				auto markers = markersNode->getMarkers();
				if (markers.empty()) {
					throw(ofxRulr::Exception("No markers available"));
				}

				auto navigateToFoundMarkers = [this, camera, &markers](const std::vector<aruco::Marker>& foundMarkers) {
					vector<cv::Point3f> worldPoints;
					vector<cv::Point2f> imagePoints;

					for (auto foundMarker : foundMarkers) {
						for (auto storedMarker : markers) {
							// Do not add ignored markers to the dataset
							if (storedMarker->parameters.ignore.get()) {
								continue;
							}

							if (storedMarker->parameters.ID.get() == foundMarker.id) {
								const auto worldPointsToAdd = ofxCv::toCv(storedMarker->getWorldVertices());
								worldPoints.insert(worldPoints.end(), worldPointsToAdd.begin(), worldPointsToAdd.end());

								const auto& imagePointsToAdd = (vector<cv::Point2f>&) foundMarker;
								imagePoints.insert(imagePoints.end(), imagePointsToAdd.begin(), imagePointsToAdd.end());
							}
						}
					}

					if (worldPoints.empty()) {
						throw(ofxRulr::Exception("No matching markers found"));
					}

					cv::Mat rotationVector, translation;
					if (this->parameters.ransac.enabled.get()) {
						if (!cv::solvePnPRansac(worldPoints
							, imagePoints
							, camera->getCameraMatrix()
							, camera->getDistortionCoefficients()
							, rotationVector
							, translation
							, this->parameters.useExtrinsicGuess.get()
							, this->parameters.ransac.maxIterations.get()
							, this->parameters.ransac.reprojectionError.get())) {
							throw(ofxRulr::Exception("Failed to solvePnPRansac"));
						}
					}
					else {
						if (!cv::solvePnP(worldPoints
							, imagePoints
							, camera->getCameraMatrix()
							, camera->getDistortionCoefficients()
							, rotationVector
							, translation
							, this->parameters.useExtrinsicGuess.get()
							, cv::SOLVEPNP_ITERATIVE)) {
							throw(ofxRulr::Exception("Failed to solvePnP"));
						}
					}

					camera->setExtrinsics(rotationVector
						, translation
						, true);
				};

				// Navigate using directly found markers
				auto foundMarkers = detector->findMarkers(image);
				navigateToFoundMarkers(foundMarkers);

				// Find missing markers and navigate using those also
				if (this->parameters.findMissingMarkers.enabled.get()) {
					// Bounds of camera image
					auto imageBounds = ofRectangle(0
						, 0
						, camera->getWidth()
						, camera->getHeight());

					auto cameraView = camera->getViewInWorldSpace();

					// Gather image points of markers that weren't detected but should be in image bounds
					map<int, vector<glm::vec2>> missingMarkerExpectedImagePoints;
					for (auto marker : markers) {
						// ignore ignored markers
						if (marker->parameters.ignore.get()) {
							continue;
						}

						// first check if the marker is missing
						{
							bool markerMissing = true;
							for (auto foundMarker : foundMarkers) {
								if (marker->parameters.ID.get() == foundMarker.id) {
									markerMissing = false;
									break;
								}
							}
							if (!markerMissing) {
								continue;
							}
						}

						// gather image points and image centroid for this marker
						auto markerImageCentroid = glm::vec2(0, 0);
						vector<glm::vec2> imagePoints;
						{
							auto worldPoints = marker->getWorldVertices();
							for (auto& worldPoint : worldPoints) {
								auto imagePoint = cameraView.getScreenCoordinateOfWorldPosition(worldPoint);
								imagePoints.emplace_back(imagePoint.x, imagePoint.y);
								markerImageCentroid += imagePoints.back();
							}
							markerImageCentroid /= worldPoints.size();
						}

						// check if within camera frustum
						if (!imageBounds.inside(markerImageCentroid)) {
							continue;
						}

						// store this as one to process
						missingMarkerExpectedImagePoints[marker->parameters.ID.get()] = imagePoints;
					}

					// Try a cropped capture for each of these missing markers
					{
						for (const auto& it : missingMarkerExpectedImagePoints) {
							// take bounding rectangle
							auto searchImageBounds = ofxCv::toOf(cv::boundingRect(ofxCv::toCv(it.second)));
							
							// expand the bounds
							{
								auto priorWidth = searchImageBounds.width;
								auto priorHeight = searchImageBounds.height;

								auto scaleFactor = this->parameters.findMissingMarkers.searchRange.get();
								searchImageBounds.width = priorWidth * scaleFactor;
								searchImageBounds.height = priorHeight * scaleFactor;

								searchImageBounds.x -= (searchImageBounds.width - priorWidth) / 2.0f;
								searchImageBounds.y -= (searchImageBounds.width - priorWidth) / 2.0f;

								// crop back to image bounds
								searchImageBounds = imageBounds.getIntersection(searchImageBounds);
							}

							// perform capture in cropped region
							{
								auto croppedImage = image(ofxCv::toCv(searchImageBounds)).clone();

								auto foundMarkersInCrop = detector->findMarkers(croppedImage);

								// Add them to our set if they are new
								for (const auto & foundMarkerInCrop : foundMarkersInCrop) {
									//Check it's not an existing one
									{
										bool alreadyFound = false;

										for (auto priorFoundMarker : foundMarkers) {
											if (foundMarkerInCrop.id == priorFoundMarker.id) {
												alreadyFound = true;
												break;
											}
										}

										if (alreadyFound) {
											continue;
										}
									}

									// Offset the cropped coordinates and add to dataset
									{
										auto foundMarkerInCropMovedBackToImage = foundMarkerInCrop;
										for (auto& imagePoint : foundMarkerInCropMovedBackToImage) {
											imagePoint += ofxCv::toCv((glm::vec2) searchImageBounds.getTopLeft());
										}
										foundMarkers.push_back(foundMarkerInCropMovedBackToImage);
									}
								}
							}
						}
					}

 					navigateToFoundMarkers(foundMarkers);
				}

				if (this->parameters.debug.speakCount.get()) {
					ofxRulr::Utils::speakCount(foundMarkers.size());
				}

				// build debug view
				{
					this->cameraRays.clear();
					auto cameraView = camera->getViewInWorldSpace();
					for (const auto& foundMarker : foundMarkers) {
						for (const auto& imagePoint : foundMarker){
							auto ray = cameraView.castPixel(ofxCv::toOf(imagePoint), true);
							ray.color = this->getColor();
							this->cameraRays.push_back(ray);
						}
					}
				}
			}

			//----------
			void NavigateCamera::populateInspector(ofxCvGui::InspectArguments& inspectArgs) {
				auto inspector = inspectArgs.inspector;

				inspector->addButton("Track fresh frame", [this]() {
					try {
						Utils::ScopedProcess scopedProcess("Track fresh frame");
						this->throwIfMissingAnyConnection();
						auto camera = this->getInput<Item::Camera>();
						auto frame = camera->getFreshFrame();
						if (!frame) {
							throw(ofxRulr::Exception("Couldn't get fresh frame"));
						}
						if (frame->getPixels().getHeight() == 0) {
							throw(ofxRulr::Exception("Pixels empty"));
						}
						auto image = ofxCv::toCv(frame->getPixels());
						this->track(image);
						scopedProcess.end();
					}
					RULR_CATCH_ALL_TO_ALERT;
					}, ' ');

				inspector->addButton("Track prior frame", [this]() {
					try {
						Utils::ScopedProcess scopedProcess("Track prior frame");
						this->throwIfMissingAnyConnection();
						auto camera = this->getInput<Item::Camera>();
						auto frame = camera->getFrame();
						if (!frame) {
							throw(ofxRulr::Exception("No camera frame available"));
						}
						if (frame->getPixels().getHeight() == 0) {
							throw(ofxRulr::Exception("Pixels empty"));
						}
						auto image = ofxCv::toCv(frame->getPixels());
						this->track(image);
						scopedProcess.end();
					}
					RULR_CATCH_ALL_TO_ALERT;
					}, OF_KEY_RETURN)->setHeight(100.0f);
			}
		}
	}
}

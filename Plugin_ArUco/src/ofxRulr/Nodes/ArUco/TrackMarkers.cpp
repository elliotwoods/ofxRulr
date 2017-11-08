#include "pch_Plugin_ArUco.h"
#include "TrackMarkers.h"

#include "Detector.h"
#include "aruco.h"

namespace ofxRulr {
	namespace Nodes {
		namespace ArUco {
			//----------
			TrackMarkers::TrackMarkers() {
				RULR_NODE_INIT_LISTENER;
				this->setIcon(Nodes::GraphicsManager::X().getIcon("ArUco::Base"));
			}

			//----------
			std::string TrackMarkers::getTypeName() const {
				return "ArUco::TrackMarkers";
			}

			//----------
			void TrackMarkers::init() {
				RULR_NODE_DRAW_WORLD_LISTENER;
				RULR_NODE_UPDATE_LISTENER;

				this->addInput<Detector>();
				this->addInput<Item::Camera>();

				this->panel = ofxCvGui::Panels::makeBaseDraws(this->previewComposite);

				this->previewPlane.clear();
				this->previewPlane.setMode(ofPrimitiveMode::OF_PRIMITIVE_TRIANGLE_FAN);
				this->previewPlane.addVertices({
					{-0.5f, +0.5f, 0.0f}
					, {-0.5f, -0.5f, 0.0f}
					, {0.5f, -0.5f, 0.0f}
					, {0.5f, +0.5f, 0.0f}
				});
				this->previewPlane.addTexCoords({
					{ 0, 0 }
					,{ 0, ARUCO_PREVIEW_RESOLUTION }
					,{ ARUCO_PREVIEW_RESOLUTION, ARUCO_PREVIEW_RESOLUTION }
					,{ ARUCO_PREVIEW_RESOLUTION, 0 }
				});
			}

			//----------
			void TrackMarkers::update() {
				auto cameraNode = this->getInput<Item::Camera>();
				auto detectorNode = this->getInput<Detector>();

				//build a multimap for this frame
				multimap<int, unique_ptr<TrackedMarker>> trackedMarkersPreviousFrame;
				std::swap(this->trackedMarkers, trackedMarkersPreviousFrame);

				this->rawMarkers.clear();
				
				if ((bool) cameraNode && (bool)detectorNode) {
					auto grabber = cameraNode->getGrabber();
					if (grabber) {
						if (grabber->isFrameNew()) {
							auto frame = grabber->getFrame();
							if (frame) {
								try {
									vector<vector<cv::Point2f>> corners;
									vector<int> ids;

									auto & pixels = frame->getPixels();
									auto image = ofxCv::toCv(pixels);
									auto & markerDetector = detectorNode->getMarkerDetector();

									auto cameraMatrix = cameraNode->getCameraMatrix();
									auto distortionCoefficients = cameraNode->getDistortionCoefficients();

									markerDetector.detect(image, this->rawMarkers);

									//update preview image
									{
										if (this->previewComposite.getWidth() != pixels.getWidth()
											|| this->previewComposite.getHeight() != pixels.getHeight()) {
											ofFbo::Settings fboSettings;
											fboSettings.width = pixels.getWidth();
											fboSettings.height = pixels.getHeight();
											fboSettings.internalformat = GL_RGBA;

											this->previewComposite.allocate(fboSettings);
											this->previewMask.allocate(fboSettings);
										}

										this->previewMask.begin();
										{
											ofClear(100, 0, 0, 255);

											ofMesh pad;
											pad.setMode(ofPrimitiveMode::OF_PRIMITIVE_TRIANGLE_FAN);

											for (auto & rawMarker : this->rawMarkers) {
												pad.clear();
												for (auto vertex : rawMarker) {
													pad.addVertex(ofxCv::toOf(vertex));
												}
												pad.drawFaces();
											}
										}
										this->previewMask.end();

										this->previewComposite.begin();
										{
											grabber->draw(0, 0);

											ofEnableBlendMode(ofBlendMode::OF_BLENDMODE_MULTIPLY);
											{
												this->previewMask.draw(0, 0);
											}
											ofDisableBlendMode();

											for (auto & marker : rawMarkers) {
												ofDrawBitmapString(ofToString(marker.id), ofxCv::toOf(marker[0]));
											}
										}
										this->previewComposite.end();
									}
									

									//calculate 3D pose and store markers
									auto markerLength = detectorNode->getMarkerLength();
									auto objectPoints = aruco::Marker::get3DPoints(markerLength);
									for (auto & rawMarker : rawMarkers) {
										unique_ptr<TrackedMarker> newMarker;

										auto findPrevious = trackedMarkersPreviousFrame.find(rawMarker.id);
										bool foundInPrevious = findPrevious != trackedMarkersPreviousFrame.end();

										if(foundInPrevious) {
											newMarker = move(findPrevious->second);
											trackedMarkersPreviousFrame.erase(findPrevious);
										}
										else {
											newMarker = make_unique<TrackedMarker>();
											newMarker->ID = rawMarker.id;
										}

										newMarker->markerLength = markerLength;

										cv::solvePnP(objectPoints
											, (vector<cv::Point2f> &)rawMarker // marker inherits from vector<cv::Point2f>
											, cameraMatrix
											, distortionCoefficients
											, newMarker->rotation
											, newMarker->translation
											, foundInPrevious);

										newMarker->transform = ofxCv::makeMatrix(newMarker->rotation, newMarker->translation) * cameraNode->getTransform();

										for (int i = 0; i < 4; i++) {
											newMarker->cornersInImage[i] = ofxCv::toOf(rawMarker[i]);
											newMarker->cornersInObjectSpace[i] = ofxCv::toOf(objectPoints[i]);
										}
										this->trackedMarkers.emplace(newMarker->ID, move(newMarker));
									}
								}
								RULR_CATCH_ALL_TO_ERROR;
							}
						}
					}
				}
			}

			//----------
			void TrackMarkers::drawWorldStage() {
				//preview markers
				auto detectorNode = this->getInput<Detector>();
				if (detectorNode) {
					for (auto & marker : this->trackedMarkers) {
						ofPushMatrix();
						{
							ofMultMatrix(marker.second->transform);

							ofPushMatrix();
							{
								ofScale(ofVec3f(marker.second->markerLength));

								auto & image = detectorNode->getMarkerImage(marker.first);

								image.bind();
								{
									this->previewPlane.draw();
								}
								image.unbind();
							}
							ofPopMatrix();

							ofDrawBitmapString(ofToString(marker.first), marker.second->cornersInObjectSpace[0]);
						}
						ofPopMatrix();
					}
				}
			}

			//----------
			ofxCvGui::PanelPtr TrackMarkers::getPanel() {
				return this->panel;
			}

			//----------
			const multimap<int, unique_ptr<TrackMarkers::TrackedMarker>> & TrackMarkers::getTrackedMarkers() const {
				return this->trackedMarkers;
			}

			//----------
			const vector<aruco::Marker> TrackMarkers::getRawMarkers() const {
				return this->rawMarkers;
			}
		}
	}
}

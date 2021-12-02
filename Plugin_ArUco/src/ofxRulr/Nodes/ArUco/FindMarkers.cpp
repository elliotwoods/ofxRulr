#include "pch_Plugin_ArUco.h"
#include "FindMarkers.h"

#include "Detector.h"

namespace ofxRulr {
	namespace Nodes {
		namespace ArUco {
			//----------
			FindMarkers::FindMarkers() {
				RULR_NODE_INIT_LISTENER;
				this->setIcon(Nodes::GraphicsManager::X().getIcon("ArUco::Base"));
			}

			//----------
			std::string FindMarkers::getTypeName() const {
				return "ArUco::FindMarkers";
			}

			//----------
			void FindMarkers::init() {
				RULR_NODE_DRAW_WORLD_LISTENER;
				RULR_NODE_UPDATE_LISTENER;
				RULR_NODE_INSPECTOR_LISTENER;

				this->addInput<Item::Camera>();
				this->addInput<Detector>();

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

				this->manageParameters(this->parameters);
			}

			//----------
			void FindMarkers::update() {
				
				{
					auto shouldProcess = isActive(this, this->parameters.detection.processWhen);
					if (shouldProcess) {
						auto cameraNode = this->getInput<Item::Camera>();
						if (cameraNode) {
							auto grabber = cameraNode->getGrabber();
							if (grabber) {
								if (grabber->isFrameNew()) {
									try {
										this->detect();
									}
									RULR_CATCH_ALL_TO_ERROR;
								}
							}
						}
					}
				}
			}

			//----------
			void FindMarkers::drawWorldStage() {
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
									this->previewPlane.draw();
								}
								image.unbind();

								// Draw outline
								ofPushStyle();
								{
									ofNoFill();
									ofSetColor(this->getColor());
									ofDrawRectangle(ofRectangle(-0.5, -0.5, 1, 1));
								}
								ofPopStyle();
							}
							ofPopMatrix();

							if (isActive(this, this->parameters.drawLabels)) {
								ofxCvGui::Utils::drawTextAnnotation(ofToString(marker.first)
									, marker.second->cornersInObjectSpace[0]
									, this->getColor());
							}
						}
						ofPopMatrix();
					}
				}
			}

			//----------
			void FindMarkers::populateInspector(ofxCvGui::InspectArguments& inspectArgs) {
				auto inspector = inspectArgs.inspector;

				inspector->addButton("Get fresh frame", [this]() {
					try {
						Utils::ScopedProcess scopedProcess("Detect fresh frame");
						this->throwIfMissingAConnection<Item::Camera>();
						auto camera = this->getInput<Item::Camera>();
						if (camera) {
							auto grabber = camera->getGrabber();
							if (!grabber) {
								throw(ofxRulr::Exception("Can't get grabber"));
							}
							grabber->getFreshFrame();
							this->detect();
						}
						scopedProcess.end();
					}
					RULR_CATCH_ALL_TO_ALERT;
					}, ' ');

				inspector->addButton("Detect", [this]() {
					try {
						Utils::ScopedProcess scopedProcess("Detect");
						this->detect();
						scopedProcess.end();
					}
					RULR_CATCH_ALL_TO_ALERT;
					}, OF_KEY_RETURN)->setHeight(100.0f);

			}


			//----------
			ofxCvGui::PanelPtr FindMarkers::getPanel() {
				return this->panel;
			}

			//----------
			void FindMarkers::detect() {
				this->throwIfMissingAnyConnection();

				auto cameraNode = this->getInput<Item::Camera>();
				auto detectorNode = this->getInput<Detector>();

				auto grabber = cameraNode->getGrabber();
				if (!grabber) {
					throw(ofxRulr::Exception("No grabber available"));
				}

				auto frame = grabber->getFrame();

				//build a multimap for this frame
				multimap<int, unique_ptr<TrackedMarker>> trackedMarkersPreviousFrame;
				std::swap(this->trackedMarkers, trackedMarkersPreviousFrame);

				vector<vector<cv::Point2f>> corners;
				vector<int> ids;

				auto& pixels = frame->getPixels();
				auto image = ofxCv::toCv(pixels);

				//perform the detection
				{
					this->rawMarkers = detectorNode->findMarkers(image, false);
				}

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

						for (auto& rawMarker : this->rawMarkers) {
							pad.clear();
							for (auto vertex : rawMarker) {
								pad.addVertex({
									vertex.x
									, vertex.y
									, 0.0f
									});
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

						for (auto& marker : this->rawMarkers) {
							ofDrawBitmapString(ofToString(marker.id), ofxCv::toOf(marker[0]));
						}
					}
					this->previewComposite.end();
				}


				//calculate 3D pose and store markers
				{
					auto cameraMatrix = cameraNode->getCameraMatrix();
					auto distortionCoefficients = cameraNode->getDistortionCoefficients();
					auto markerLength = detectorNode->getMarkerLength();
					auto objectPoints = aruco::Marker::get3DPoints(markerLength);
					for (auto& rawMarker : this->rawMarkers) {
						unique_ptr<TrackedMarker> newMarker;

						auto findPrevious = trackedMarkersPreviousFrame.find(rawMarker.id);
						bool foundInPrevious = findPrevious != trackedMarkersPreviousFrame.end();

						if (foundInPrevious) {
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

						newMarker->transform = cameraNode->getTransform()
							* ofxCv::makeMatrix(newMarker->rotation, newMarker->translation);

						for (int i = 0; i < 4; i++) {
							newMarker->cornersInImage[i] = ofxCv::toOf(rawMarker[i]);
							newMarker->cornersInObjectSpace[i] = ofxCv::toOf(objectPoints[i]);
						}
						this->trackedMarkers.emplace(newMarker->ID, move(newMarker));
					}
				}

				//save the image
				{
					auto shouldSave = this->parameters.save.whenToSave.get() == WhenToSave::Always
						|| (this->parameters.save.whenToSave.get() == WhenToSave::Success && !this->trackedMarkers.empty());

					if (shouldSave) {
						auto saveFolder = this->parameters.save.folder.get();
						if (saveFolder.empty()) {
							throw(ofxRulr::Exception("No save path selected"));
						}
						static size_t index = 0;
						auto saveFile = saveFolder;
						saveFile.append(ofToString(index++) + ".png");
						auto pathString = saveFile.string();
						if (pathString.size() > 2 && pathString[0] == '"') {
							//strip quotes
							pathString = pathString.substr(1, pathString.size() - 2);
						}
						ofSaveImage(grabber->getPixels(), pathString);
					}
				}

				//speak the count
				if (this->parameters.detection.speakCount) {
					Utils::speakCount(this->trackedMarkers.size());
				}
			}

			//----------
			const multimap<int, unique_ptr<FindMarkers::TrackedMarker>> & FindMarkers::getTrackedMarkers() const {
				return this->trackedMarkers;
			}

			//----------
			const vector<aruco::Marker> FindMarkers::getRawMarkers() const {
				return this->rawMarkers;
			}
		}
	}
}

#include "pch_Plugin_ArUco.h"
#include "ofxRulr/Solvers/MarkerProjections.h"

namespace ofxRulr {
	namespace Nodes {
		namespace MarkerMap {
#pragma mark Capture
			//----------
			Calibrate::Capture::Capture()
			{
				RULR_SERIALIZE_LISTENERS;
				RULR_NODE_INSPECTOR_LISTENER;
			}

			//----------
			string
				Calibrate::Capture::getDisplayString() const
			{
				stringstream ss;
				if (!this->name.get().empty()) {
					ss << this->name.get() << " : ";
				}
				for (const auto& ID : this->IDs) {
					ss << ID << ", ";
				}
				if (!this->residuals.empty()) {
					ss << std::endl;
					for (const auto& residual : this->residuals) {
						ss << ofToString(residual, 2) << ", ";
					}
				}
				return ss.str();
			}
			//---------
			ofxCvGui::ElementPtr
				Calibrate::Capture::getDataDisplay()
			{
				// ID
				// Select / inspect
				// Position
				auto elementGroup = ofxCvGui::makeElementGroup();
				this->dataDisplay = elementGroup;
				
				this->refreshDataDisplay();

				return elementGroup;
			}

			//----------
			void
				Calibrate::Capture::update(const UpdateArgs& args)
			{
				bool hasIssue = false;

				// Check if camera position is within room bounds
				{
					auto position = this->cameraView.getPosition();
					hasIssue |= position.x < args.roomMin.x;
					hasIssue |= position.y < args.roomMin.y;
					hasIssue |= position.z < args.roomMin.z;
					hasIssue |= position.x > args.roomMax.x;
					hasIssue |= position.y > args.roomMax.y;
					hasIssue |= position.z > args.roomMax.z;
					this->hasIssue = hasIssue;
				}

				// Store residuals to use in previews
				{
					this->maxResidual = args.maxResidual;
				}

				if (args.refreshDataDisplay) {
					this->refreshDataDisplay();
				}
			}

			//----------
			void
				Calibrate::Capture::refreshDataDisplay()
			{
				auto elementGroup = this->dataDisplay.lock();
				if (!elementGroup) {
					return;
				}

				auto y = 0;

				vector<shared_ptr<ofxCvGui::Element>> widgets;

				// Top row group
				{
					auto group = ofxCvGui::makeElementGroup();
					vector<shared_ptr<ofxCvGui::Element>> groupWidgets;

					// Inspect
					{
						auto widget = make_shared<ofxCvGui::Widgets::Toggle>("Inspect"
							, [this]() {
								return this->isBeingInspected();
							}
							, [this](bool value) {
								if (value) {
									ofxCvGui::inspect(this->shared_from_this());
								}
							});
						widget->setDrawGlyph(u8"\uf101");
						group->add(widget);
						groupWidgets.push_back(widget);
					}

					// Initialised
					{
						auto widget = make_shared<ofxCvGui::Widgets::Toggle>("Initialised"
							, [this]() {
								return this->initialised;
							}
							, [this](bool value) {
								this->initialised = value;
							});
						widget->setDrawGlyph(u8"\uf00c");
						group->add(widget);
						groupWidgets.push_back(widget);
					}

					// Markers
					for (size_t i = 0; i < IDs.size(); i++) {
						{

							auto widget = ofxCvGui::makeElement();
							widget->onDraw += [this, i](ofxCvGui::DrawArguments& args) {
								auto ID = this->IDs[i];
								auto hasResidual = this->residuals.size() > i;

								// Data that needs other nodes
								if (this->parent) {
									auto markersNode = this->parent->getInput<Markers>();
									if (markersNode) {
										// Draw the marker preview
										auto detector = markersNode->getInput<ArUco::Detector>();
										if (detector) {
											auto size = min(args.localBounds.getHeight(), args.localBounds.getWidth());
											const auto& markerImage = detector->getMarkerImage(ID);
											ofPushStyle();
											{
												ofSetColor(100); // Dim the marker view (so we can see text)
												markerImage.draw(args.localBounds.getCenter() - glm::vec2(size, size) / 2.0f
													, size
													, size);
											}
											ofPopStyle();
										}

										// Draw an indicator if this marker is already seen elsewhere
										try {
											if (markersNode->getMarkerByID(ID)) {
												// Marker already exists in set
												ofPushStyle();
												{
													ofSetColor(100, 255, 100);
													ofDrawCircle(args.localBounds.getBottomRight() + glm::vec2(-4, -4), 3);
												}
												ofPopStyle();
											}
										}
										catch (...) {

										}
									}
								}

								// Draw the reprojection bar
								if (hasResidual) {
									ofPushStyle();
									{
										ofSetColor(255, 0, 0);

										auto residual = this->residuals[i];
										auto residualNorm = residual / this->maxResidual;
										auto y = args.localBounds.height * (1.0f - residualNorm);
										ofDrawRectangle(0
											, y
											, 5
											, args.localBounds.height - y);
									}
									ofPopStyle();
								}

								// Draw the marker index
								{
									ofxCvGui::Utils::drawText(ofToString(ID), args.localBounds, false);
								}
							};

							{
								auto ID = this->IDs[i];
								auto hasResidual = this->residuals.size() > i;
								if (hasResidual) {
									auto residual = this->residuals[i];
									widget->addToolTip(ofToString(ID) + " (" + ofToString(residual) + ")");
								}
								else {
									widget->addToolTip(ofToString(ID));
								}
							}
							group->add(widget);
							groupWidgets.push_back(widget);
						}
					}

					group->setHeight(50);
					group->onBoundsChange += [groupWidgets](ofxCvGui::BoundsChangeArguments& args) {
						auto itemWidth = args.localBounds.width / groupWidgets.size();
						auto height = args.localBounds.height;
						auto x = 0;
						for (auto widget : groupWidgets) {
							widget->setBounds(ofRectangle(x, 0, itemWidth - 5, height));
							x += itemWidth;
						}
					};

					y += group->getHeight();
					elementGroup->add(group);
					widgets.push_back(group);
				}

				elementGroup->setHeight(y + 5);
				elementGroup->onBoundsChange += [widgets](ofxCvGui::BoundsChangeArguments& args) {
					for (auto widget : widgets) {
						widget->setWidth(args.localBounds.width);
					}
				};

				elementGroup->onDraw += [this](ofxCvGui::DrawArguments& args)
				{
					if (this->hasIssue) {
						ofPushStyle();
						{
							ofSetColor(255, 0, 0);
							ofNoFill();
							ofDrawRectangle(args.localBounds);
						}
						ofPopStyle();
					}
				};
			}

			//----------
			void
				Calibrate::Capture::drawWorld()
			{
				bool isHighlighted = false;

				// Check if the mouse is over element in capture list
				{
					auto dataDisplay = this->dataDisplay.lock();
					if (dataDisplay) {
						isHighlighted |= dataDisplay->isMouseOver();
					}
				}

				// Check if is being inspected
				if (this->isBeingInspected()) {
					isHighlighted = true;
				}

				if (isHighlighted) {
					ofxCvGui::Utils::drawTextAnnotation(this->name, this->cameraView.getPosition(), this->color);
				}
			}

			//----------
			void
				Calibrate::Capture::serialize(nlohmann::json& json) const
			{
				Utils::serialize(json, this->name);
				Utils::serialize(json, "IDs", this->IDs);
				Utils::serialize(json, "imagePoints", this->imagePoints);
				Utils::serialize(json, "imagePointsUndistorted", this->imagePointsUndistorted);
				Utils::serialize(json, "residuals", this->residuals);
				Utils::serialize(json, "cameraView", this->cameraView);
				Utils::serialize(json, "cameraRays", this->cameraRays);
				Utils::serialize(json, "initialised", this->initialised);
			}

			//----------
			void
				Calibrate::Capture::deserialize(const nlohmann::json& json)
			{
				Utils::deserialize(json, this->name);
				Utils::deserialize(json, "IDs", this->IDs);
				Utils::deserialize(json, "imagePoints", this->imagePoints);
				Utils::deserialize(json, "imagePointsUndistorted", this->imagePointsUndistorted);
				Utils::deserialize(json, "residuals", this->residuals);
				Utils::deserialize(json, "cameraView", this->cameraView);
				Utils::deserialize(json, "initialised", this->initialised);
			}

			//----------
			void
				Calibrate::Capture::populateInspector(ofxCvGui::InspectArguments& args)
			{
				auto inspector = args.inspector;

				inspector->addToggle("Initialised"
					, [this]() {
						return this->initialised;
					}
					, [this](bool value) {
						this->initialised = value;
					});

				inspector->addEditableValue<glm::vec3>("Position"
					, [this]() {
						return this->cameraView.getPosition();
					}
					, [this](const string& valueString) {
						if (!valueString.empty()) {
							stringstream ss(valueString);
							glm::vec3 value;
							ss >> value;
							this->cameraView.setPosition(value);
						}
					});

				inspector->addEditableValue<glm::vec3>("Rotation"
					, [this]() {
						auto quat = this->cameraView.getOrientationQuat();
						return glm::eulerAngles(quat);
					}
					, [this](const string& valueString) {
						if (!valueString.empty()) {
							stringstream ss(valueString);
							glm::vec3 value;
							ss >> value;
							glm::quat quat(value); // convert from euler to quat
							this->cameraView.setOrientation(quat);
						}
					});

				inspector->addButton("Reset transform", [this]() {
					this->cameraView.resetTransform();
					});

				// Image view
				{
					auto view = ofxCvGui::makeElement();
					view->setWidth(200);
					view->setHeight(200);
					view->onDraw += [this](ofxCvGui::DrawArguments& args) {
						// Draw outline
						ofPushStyle();
						{
							ofNoFill();
							ofDrawRectangle(args.localBounds);
						}
						ofPopStyle();

						// Scale the view for camera coords
						ofPushMatrix();
						{
							ofScale(args.localBounds.width / this->cameraView.getWidth()
								, args.localBounds.height / this->cameraView.getHeight());


							// Draw markers
							ofPushStyle();
							{
								ofNoFill();

								auto size = this->IDs.size();
								if (size == this->imagePointsUndistorted.size()) {
									for (size_t i = 0; i < size; i++) {
										const auto& ID = this->IDs[i];
										const auto& imagePointsUndistorted = this->imagePointsUndistorted[i];

										// Draw outline
										{
											ofPolyline line;
											for (auto& vertex : imagePointsUndistorted) {
												line.addVertex(glm::vec3(vertex, 0.0f));
											}
											line.close();
											line.draw();
										}

										// Draw label at origin
										{
											if (!imagePointsUndistorted.empty()) {
												ofDrawBitmapStringHighlight(ofToString(ID), imagePointsUndistorted.front());
											}
										}

									}
								}
							}
							ofPopStyle();
						}
						ofPopMatrix();
					};
					inspector->add(view);
				}
			}

			//----------
			string Calibrate::Capture::getName() const {
				return this->name;
			}

#pragma mark Calibrate
			//----------
			Calibrate::Calibrate() {
				RULR_NODE_INIT_LISTENER;
			}

			//----------
			string Calibrate::getTypeName() const {
				return "MarkerMap::Calibrate";
			}

			//----------
			void Calibrate::init() {
				RULR_NODE_DRAW_WORLD_LISTENER;
				RULR_NODE_UPDATE_LISTENER;
				RULR_NODE_INSPECTOR_LISTENER;
				RULR_NODE_SERIALIZATION_LISTENERS;

				this->manageParameters(this->parameters);

				{
					this->panel = ofxCvGui::Panels::makeWidgets();
					this->captures.populateWidgets(this->panel);
					this->panel->addLiveValue<size_t>("Total markers in selection", [this]() {
						auto selection = this->captures.getSelection();
						size_t count = 0;
						for (auto capture : selection) {
							count += capture->IDs.size();
						}
						return count;
						});
				}

				this->addInput<Item::Camera>();
				this->addInput<Markers>();
			}

			//----------
			void Calibrate::update() {
				if (this->dirty.capturePreviews) {
					this->updateCapturePreviews();
				}
			}

			//----------
			void Calibrate::drawWorldStage() {
				auto captures = this->captures.getSelection();
				for (auto capture : captures) {
					capture->drawWorld();

					if (this->parameters.draw.cameraRays.get()) {
						for (const auto& cameraRay : capture->cameraRays) {
							cameraRay.draw();
						}
					}

					if (this->parameters.draw.cameraViews.get()) {
						capture->cameraView.draw();
					}

					if (this->parameters.draw.labels.get()) {
						auto name = capture->getName();
						if (!name.empty()) {
							ofxCvGui::Utils::drawTextAnnotation(name
								, capture->cameraView.getPosition()
								, capture->color);
						}
					}
				}
			}

			//----------
			void Calibrate::populateInspector(ofxCvGui::InspectArguments& inspectArgs) {
				auto inspector = inspectArgs.inspector;

				inspector->addButton("Capture", [this]() {
					try {
						Utils::ScopedProcess scopedProcess("Capture");
						this->capture();
						scopedProcess.end();
					}
					RULR_CATCH_ALL_TO_ALERT;
					}, ' ');
				inspector->addButton("Add folder of images", [this]() {
					try {
						Utils::ScopedProcess scopedProcess("Add folder of images");
						this->addFolderOfImages();
						scopedProcess.end();
					}
					RULR_CATCH_ALL_TO_ALERT;
					});
				inspector->addButton("Calibrate selected", [this]() {
					try {
						Utils::ScopedProcess scopedProcess("Calibrate");
						this->calibrateSelected();
						scopedProcess.end();
					}
					RULR_CATCH_ALL_TO_ERROR;
					}, OF_KEY_RETURN)->setHeight(100.0f);
				inspector->addButton("Calibrate (legacy)", [this]() {
					try {
						Utils::ScopedProcess scopedProcess("Calibrate");
						this->calibrate();
						scopedProcess.end();
					}
					RULR_CATCH_ALL_TO_ERROR;
					});
				inspector->addTitle("Progressive Markers Calibrate", ofxCvGui::Widgets::Title::Level::H3);
				inspector->addButton("Once", [this]() {
					try {
						Utils::ScopedProcess scopedProcess("Calibrate");
						this->calibrateProgressiveMarkers();
						scopedProcess.end();
					}
					RULR_CATCH_ALL_TO_ERROR;
					});
				inspector->addButton("Continuously", [this]() {
					try {
						Utils::ScopedProcess scopedProcess("Calibrate");
						this->calibrateProgressiveMarkersContinuously();
						scopedProcess.end();
					}
					RULR_CATCH_ALL_TO_ALERT;
					});
			}

			//----------
			void Calibrate::serialize(nlohmann::json& json) {
				this->captures.serialize(json["captures"]);
			}

			//----------
			void Calibrate::deserialize(const nlohmann::json& json) {
				if (json.contains("captures")) {
					this->captures.deserialize(json["captures"]);

					// Set the parent to this
					{
						auto allCaptures = this->captures.getAllCaptures();
						for (auto capture : allCaptures) {
							capture->parent = this;
						}
					}
				}

				this->dirty.capturePreviews = true;
			}

			//----------
			ofxCvGui::PanelPtr Calibrate::getPanel() {
				return this->panel;
			}

			//----------
			void Calibrate::capture() {
				this->throwIfMissingAConnection<Item::Camera>();
				auto camera = this->getInput<Item::Camera>();
				auto frame = camera->getFreshFrame();
				if (!frame) {
					throw(ofxRulr::Exception("Failed to get frame from camera"));
				}

				auto image = ofxCv::toCv(frame->getPixels());
				this->add(image, "");
			}

			//----------
			void Calibrate::calibrate() {
				this->throwIfMissingAnyConnection();
				auto camera = this->getInput<Item::Camera>();
				auto markersNode = this->getInput<Markers>();
				markersNode->throwIfMissingAnyConnection();
				auto detectorNode = markersNode->getInput<ArUco::Detector>();

				auto initialisationMarkers = markersNode->getMarkers();
				auto activeCaptures = this->captures.getSelection();

				// Markers initialised during this session which weren't in the set before
				vector<shared_ptr<Markers::Marker>> markersWeInitialised;

				if (initialisationMarkers.empty()) {
					throw(ofxRulr::Exception("No markers selected"));
				}

				if (activeCaptures.empty()) {
					throw(ofxRulr::Exception("No captures selected"));
				}

				// Find the fixed markers
				vector<shared_ptr<Markers::Marker>> fixedMarkers;
				{
					for (auto marker : initialisationMarkers) {
						if (marker->parameters.fixed.get()) {
							fixedMarkers.push_back(marker);
						}
					}

					if (fixedMarkers.empty()) {
						throw(ofxRulr::Exception("No fixed markers found. You should fix at least one marker in position before solving"));
					}
				}

				// Function for searching
				auto captureContainsID = [](shared_ptr<Capture> capture, int ID) {
					return std::find(capture->IDs.begin()
						, capture->IDs.end()
						, ID)
						!= capture->IDs.end();
				};
				auto findCapturesContainingID = [&](int ID) {
					std::vector<shared_ptr<Capture>> captures;
					for (auto capture : activeCaptures) {
						if (captureContainsID(capture, ID)) {
							captures.push_back(capture);
						}
					}
					return captures;
				};
				auto isFixedMarker = [&](int ID) {
					for (auto marker : fixedMarkers) {
						if (marker->parameters.ID.get() == ID) {
							return true;
						}
					}
					return false;
				};
				auto markerHasBeenInitialised = [&](int ID) {
					for (auto marker : initialisationMarkers) {
						if (marker->parameters.ID.get() == ID) {
							return true;
						}
					}
					for (auto marker : markersWeInitialised) {
						if (marker->parameters.ID.get() == ID) {
							return true;
						}
					}
					return false;
				};
				auto getMarker = [&](int ID, bool create) {
					for (auto marker : initialisationMarkers) {
						if (marker->parameters.ID.get() == ID) {
							return marker;
						}
					}
					for (auto marker : markersWeInitialised) {
						if (marker->parameters.ID.get() == ID) {
							return marker;
						}
					}

					if (create) {
						auto marker = make_shared<Markers::Marker>();
						marker->parameters.ID.set(ID);
						marker->parameters.length.set(detectorNode->getMarkerLength());

						// Note that this function sets length and parent
						markersNode->add(marker);
						markersWeInitialised.push_back(marker);
						return marker;
					}
					else {
						throw(ofxRulr::Exception("Marker " + ofToString(ID) + " not found"));
					}
				};
				auto allMarkersInCaptureInitialised = [&](shared_ptr<Capture> capture) {
					for (auto ID : capture->IDs) {
						if (!markerHasBeenInitialised(ID)) {
							return false;
						}
					}
					return true;
				};
				auto allCapturedMarkersInitialised = [&]() {
					for (auto capture : activeCaptures) {
						if (!allMarkersInCaptureInitialised(capture)) {
							return false;
						}
					}
					return true;
				};

				auto initialiseUnseenMarkers = [&](shared_ptr<Capture> capture) {
					auto size = capture->IDs.size();

					// Navigate the camera
					cv::Mat cameraRotationVector, cameraTranslation;
					{
						// Gather navigation image points and world points
						vector<cv::Point2f> imagePoints;
						vector<cv::Point3f> worldPoints;

						for (int i = 0; i < size; i++) {
							const auto& ID = capture->IDs[i];
							if (markerHasBeenInitialised(ID)) {
								// Add fixed marker to navigation data
								const auto& markerImagePoints = ofxCv::toCv(capture->imagePoints[i]);
								imagePoints.insert(imagePoints.end(), markerImagePoints.begin(), markerImagePoints.end());

								auto marker = getMarker(ID, false);
								const auto markerWorldPoints = ofxCv::toCv(marker->getWorldVertices());
								worldPoints.insert(worldPoints.end(), markerWorldPoints.begin(), markerWorldPoints.end());
							}
						}

						if (imagePoints.empty()) {
							throw(ofxRulr::Exception("No navigation data found"));
						}

						std::vector<int> inliers;
						cv::solvePnP(worldPoints
							, imagePoints
							, camera->getCameraMatrix()
							, camera->getDistortionCoefficients()
							, cameraRotationVector
							, cameraTranslation);

						camera->setExtrinsics(cameraRotationVector
							, cameraTranslation
							, true);
					}

					auto cameraTransform = camera->getTransform();

					// Insert all uninitialised markers into the Markers set
					{
						for (int i = 0; i < size; i++) {
							const auto& ID = capture->IDs[i];
							if (!markerHasBeenInitialised(ID)) {
								auto marker = getMarker(ID, true);
								const auto& imagePoints = ofxCv::toCv(capture->imagePoints[i]);
								const auto objectPoints = ofxCv::toCv(marker->getObjectVertices());

								cv::Mat markerRotationVector, markerTranslation;
								cv::solvePnP(objectPoints
									, imagePoints
									, camera->getCameraMatrix()
									, camera->getDistortionCoefficients()
									, markerRotationVector
									, markerTranslation);

								marker->rigidBody->setExtrinsics(markerRotationVector
									, markerTranslation
									, false);

								// And apply the camera transform on top
								auto transform = cameraTransform * marker->rigidBody->getTransform();
								marker->rigidBody->setTransform(transform);
							}
						}
					}
				};

				// Find the genesis capture
				// Includes all fixed markers, and has the maximum count
				shared_ptr<Capture> genesisCapture;
				{
					size_t maxCount = 0;
					for (auto capture : activeCaptures) {
						bool containsAllMarkers = true;
						for (auto marker : fixedMarkers) {
							if (!captureContainsID(capture, marker->parameters.ID.get())) {
								containsAllMarkers = false;
								break;
							}
						}
						if (containsAllMarkers) {
							if (capture->IDs.size() > maxCount) {
								genesisCapture = capture;
								maxCount = capture->IDs.size();
							}
						}
					}
				}

				// Position all markers in the genesis capture
				if(genesisCapture) {
					Utils::ScopedProcess scopedProcessGenesis("Process genesis capture", false);
					initialiseUnseenMarkers(genesisCapture);
				}

				// Initialise all markers
				{
					Utils::ScopedProcess scopedProcessInitialiseAllMarkers("Initialise all markers", false);
					while (!allCapturedMarkersInitialised()) {
						// Find capture with most initialised markers
						shared_ptr<Capture> captureWithMostInitialisedMarkers;
						{
							size_t maxInitialisedMarkerCount = 0;
							for (auto capture : activeCaptures) {
								size_t countInitialised = 0;
								for (auto ID : capture->IDs) {
									if (markerHasBeenInitialised(ID)) {
										countInitialised++;
									}
								}
								if (countInitialised == capture->IDs.size()) {
									// ignore if everything initialised here
									continue;
								}
								if (countInitialised > maxInitialisedMarkerCount) {
									captureWithMostInitialisedMarkers = capture;
									maxInitialisedMarkerCount = countInitialised;
								}
							}
						}

						initialiseUnseenMarkers(captureWithMostInitialisedMarkers);
					}
				}

				// Get all view transforms into captures
				{
					Utils::ScopedProcess scopedProcessGetViews("Gather all view transforms", false);

					for (auto capture : activeCaptures) {
						auto size = capture->IDs.size();

						// Navigate the camera
						cv::Mat cameraRotationVector, cameraTranslation;
						{
							// Gather navigation image points and world points
							vector<cv::Point2f> imagePoints;
							vector<cv::Point3f> worldPoints;

							for (int i = 0; i < size; i++) {
								const auto& ID = capture->IDs[i];
								const auto& markerImagePoints = ofxCv::toCv(capture->imagePoints[i]);
								imagePoints.insert(imagePoints.end(), markerImagePoints.begin(), markerImagePoints.end());

								auto marker = getMarker(ID, false);
								const auto markerWorldPoints = ofxCv::toCv(marker->getWorldVertices());
								worldPoints.insert(worldPoints.end(), markerWorldPoints.begin(), markerWorldPoints.end());
							}

							cv::solvePnP(worldPoints
								, imagePoints
								, camera->getCameraMatrix()
								, camera->getDistortionCoefficients()
								, cameraRotationVector
								, cameraTranslation);

							camera->setExtrinsics(cameraRotationVector
								, cameraTranslation
								, true);
							capture->cameraView = camera->getViewInWorldSpace();
							capture->cameraView.setFarClip(this->parameters.debug.cameraFarPlane.get());
							capture->cameraView.color = capture->color;
							capture->initialised = true;
						}
					}
				}

				// Perform the bundle adjustment solve
				vector<int> markerIDs;
				{
					Utils::ScopedProcess scopedProcessBundleAdjustment("Bundle adjustment", false);

					std::vector<vector<glm::vec3>> objectPoints;
					vector<Solvers::MarkerProjections::Image> images;
					Solvers::MarkerProjections::Solution initialSolution;
					vector<int> fixedObjectIndices;

					// Gather data
					{
						// gather marker IDs
						{
							for (auto marker : initialisationMarkers) {
								if (marker->parameters.ignore.get()) {
									continue;
								}
								markerIDs.push_back(marker->parameters.ID.get());
							}
							for (auto marker : markersWeInitialised) {
								if (marker->parameters.ignore.get()) {
									continue;
								}
								markerIDs.push_back(marker->parameters.ID.get());
							}
						}

						// gather objectPoints and object transforms
						for (auto markerID : markerIDs) {
							auto marker = getMarker(markerID, false);
							objectPoints.push_back(marker->getObjectVertices());
							initialSolution.objects.push_back(Solvers::MarkerProjections::getTransform(marker->rigidBody->getTransform()));
						}

						// gather images and view transforms
						int viewIndex = 0;
						for (auto capture : activeCaptures) {
							auto size = capture->IDs.size();

							// Serialization error previously
							if (capture->imagePointsUndistorted.empty()) {
								for (auto& imagePoints : capture->imagePoints) {
									auto undistortedImagePoints = ofxCv::undistortImagePoints(ofxCv::toCv(imagePoints)
										, camera->getCameraMatrix()
										, camera->getDistortionCoefficients());
									capture->imagePointsUndistorted.push_back(ofxCv::toOf(undistortedImagePoints));
								}
							}

							for (size_t i = 0; i < size; i++) {
								Solvers::MarkerProjections::Image image;
								image.imagePointsUndistorted = capture->imagePointsUndistorted[i];
								image.viewIndex = viewIndex;
								image.objectIndex = -1;
								for (int objectIndex = 0; objectIndex < markerIDs.size(); objectIndex++) {
									if (markerIDs[objectIndex] == capture->IDs[i]) {
										image.objectIndex = objectIndex;
										break;
									}
								}
								if (image.objectIndex == -1) {
									throw(ofxRulr::Exception("Marker not found when collating object indices"));
								}
								images.push_back(image);
							}

							auto transform = capture->cameraView.getGlobalTransformMatrix();
							auto viewTransform = glm::inverse(transform);
							auto transform2 = Solvers::MarkerProjections::getTransform(viewTransform);
							initialSolution.views.push_back(transform2);
							viewIndex++;
						}

						// gather fixed object indices
						for (size_t i = 0; i < markerIDs.size(); i++) {
							auto marker = getMarker(markerIDs[i], false);
							if (marker->parameters.fixed.get()) {
								fixedObjectIndices.push_back(i);
							}
						}
					}

					// Perform solve and unpack
					{
						auto unpackSolution = [&](Solvers::MarkerProjections::Solution& solution) {
							for (size_t i = 0; i < activeCaptures.size(); i++) {
								auto viewTransform = Solvers::MarkerProjections::getTransform(solution.views[i]);
								auto rigidBodyTransform = glm::inverse(viewTransform);

								auto rigidBodyTransform2 = Solvers::MarkerProjections::getTransform(rigidBodyTransform);

								activeCaptures[i]->cameraView.setPosition(rigidBodyTransform2.translation);

								auto orientation = ofxCeres::VectorMath::eulerToQuat(rigidBodyTransform2.rotation);
								activeCaptures[i]->cameraView.setOrientation(orientation);
							}

							for (size_t i = 0; i < markerIDs.size(); i++) {
								auto marker = getMarker(markerIDs[i], false);
								marker->rigidBody->setTransform(Solvers::MarkerProjections::getTransform(solution.objects[i]));
							}

							// Unproject camera rays for image points
							for (auto capture : activeCaptures) {
								capture->cameraRays.clear();
								for (const auto& imagePointsUndistorted : capture->imagePointsUndistorted) {
									vector<ofxRay::Ray> cameraRays;
									capture->cameraView.castPixels(imagePointsUndistorted, cameraRays, false);
									for (auto& ray : cameraRays) {
										ray.color = capture->color;
										ray.t = glm::normalize(ray.t) * this->parameters.debug.cameraRayLength.get();
										capture->cameraRays.push_back(ray);
									}
								}
							}

							// Get residuals per image
							if (solution.reprojectionErrorPerImage.size() == images.size()) {
								for (auto capture : activeCaptures) {
									capture->residuals.clear();
								}
								for (size_t i = 0; i < images.size(); i++) {
									auto& image = images[i];
									activeCaptures[image.viewIndex]->residuals.push_back(solution.reprojectionErrorPerImage[i]);
								}
							}
						};

						// Perform solve
						if (this->parameters.calibration.bundleAdjustment.enabled.get()) {
							auto solverSettings = Solvers::MarkerProjections::defaultSolverSettings();
							solverSettings.options.max_num_iterations = this->parameters.calibration.bundleAdjustment.maxIterations.get();
							solverSettings.options.function_tolerance = this->parameters.calibration.bundleAdjustment.functionTolerance.get();
							solverSettings.options.num_threads = this->parameters.calibration.bundleAdjustment.numThreads.get();

							auto cameraView = camera->getViewInObjectSpace();

							auto result = Solvers::MarkerProjections::solve(camera->getWidth()
								, camera->getHeight()
								, cameraView.getClippedProjectionMatrix()
								, objectPoints
								, images
								, fixedObjectIndices
								, initialSolution
								, solverSettings);

							if (!result.isConverged()) {
								if (this->parameters.calibration.bundleAdjustment.useIncompleteSolution.get()) {
									unpackSolution(result.solution);
								}
								throw(ofxRulr::Exception("Failed to converge"));
							}

							unpackSolution(result.solution);
						}
						else {
							if (this->parameters.debug.unpackInitial) {
								unpackSolution(initialSolution);
							}
						}
					}
				}

				this->dirty.capturePreviews = true;
			}

			//----------
			void Calibrate::calibrateSelected() {
				Utils::ScopedProcess scopedProcess("Calibrate selected");
				this->throwIfMissingAConnection<Markers>();
				this->throwIfMissingAConnection<Item::Camera>();
				auto camera = this->getInput<Item::Camera>();
				auto markersNode = this->getInput<Markers>();
				auto markers = markersNode->getMarkers();

				auto captures = this->captures.getSelection();

				// Fix prior serialisation errors
				{
					for (auto capture : captures) {
						// Serialization error previously
						if (capture->imagePointsUndistorted.empty()) {
							for (auto& imagePoints : capture->imagePoints) {
								auto undistortedImagePoints = ofxCv::undistortImagePoints(ofxCv::toCv(imagePoints)
									, camera->getCameraMatrix()
									, camera->getDistortionCoefficients());
								capture->imagePointsUndistorted.push_back(ofxCv::toOf(undistortedImagePoints));
							}
						}
					}
				}

				// Initialise captures against seen markers
				for (auto capture : captures) {
					if (!capture->initialised) {
						this->initialiseCaptureViewWithSeenMarkers(capture);
					}
				}

				// Initialise markers with existing captures
				for (auto capture : captures) {
					this->initialiseUnseenMarkersInView(capture);
				}

				// Strip out ignored markers
				{
					vector<shared_ptr<Markers::Marker>> allMarkers = markers;
					markers.clear();
					for (auto marker : allMarkers) {
						if (!marker->parameters.ignore.get()) {
							markers.push_back(marker);
						}
					}
				}

				// Gather data
				// Prepare the data
				std::vector<vector<glm::vec3>> objectPoints;
				vector<Solvers::MarkerProjections::Image> images;
				Solvers::MarkerProjections::Solution initialSolution;
				vector<int> fixedObjectIndices;
				{
					for (auto marker : markers) {
						objectPoints.push_back(marker->getObjectVertices());
						initialSolution.objects.push_back(Solvers::MarkerProjections::getTransform(marker->rigidBody->getTransform()));
					}

					// gather images and view transforms from captures
					int viewIndex = 0;
					for (auto capture : captures) {
						for (int i = 0; i < capture->IDs.size(); i++) {
							// check the markerID is in the set of markers that we are calibrating
							bool isPresent = false;
							auto markerID = capture->IDs[i];
							for (auto marker : markers) {
								if (marker->parameters.ID.get() == markerID) {
									isPresent = true;
									break;
								}
							}
							if (!isPresent) {
								// ignore this marker (which was seen in this view)
								continue;
							}

							Solvers::MarkerProjections::Image image;
							image.imagePointsUndistorted = capture->imagePointsUndistorted[i];
							image.viewIndex = viewIndex;
							image.objectIndex = -1;
							for (int objectIndex = 0; objectIndex < markers.size(); objectIndex++) {
								if (markers[objectIndex]->parameters.ID.get() == capture->IDs[i]) {
									image.objectIndex = objectIndex;
									break;
								}
							}
							if (image.objectIndex != -1) {
								images.push_back(image);
							}
						}

						auto transform = capture->cameraView.getGlobalTransformMatrix();
						auto viewTransform = glm::inverse(transform);
						auto transform2 = Solvers::MarkerProjections::getTransform(viewTransform);
						initialSolution.views.push_back(transform2);

						viewIndex++;
					}

					// gather fixed object indices
					for (size_t i = 0; i < markers.size(); i++) {
						auto marker = markers[i];
						if (marker->parameters.fixed.get()) {
							fixedObjectIndices.push_back(i);
						}
					}
				}

				// Perform solve
				{
					auto solverSettings = Solvers::MarkerProjections::defaultSolverSettings();
					solverSettings.options.max_num_iterations = this->parameters.calibration.bundleAdjustment.maxIterations.get();
					solverSettings.options.function_tolerance = this->parameters.calibration.bundleAdjustment.functionTolerance.get();
					solverSettings.options.num_threads = this->parameters.calibration.bundleAdjustment.numThreads.get();

					auto cameraView = camera->getViewInObjectSpace();

					auto result = Solvers::MarkerProjections::solve(camera->getWidth()
						, camera->getHeight()
						, cameraView.getClippedProjectionMatrix()
						, objectPoints
						, images
						, fixedObjectIndices
						, initialSolution
						, solverSettings);

					if (!result.isConverged()) {
						if (this->parameters.calibration.bundleAdjustment.useIncompleteSolution.get()) {
							unpackSolution(captures
								, result.solution
								, markers
								, images);
						}
						throw(ofxRulr::Exception("Failed to converge"));
					}

					unpackSolution(captures
						, result.solution
						, markers
						, images);
				}

				this->dirty.capturePreviews = true;
			}

			//----------
			void Calibrate::calibrateProgressiveMarkers() {
				Utils::ScopedProcess scopedProcess("Calibrate with strategy 'Progressive Markers'");

				this->throwIfMissingAConnection<Markers>();
				this->throwIfMissingAConnection<Item::Camera>();
				auto camera = this->getInput<Item::Camera>();
				auto markersNode = this->getInput<Markers>();
				auto preInitialisedMarkers = markersNode->getMarkers();

				// Make a set of seen marker IDs
				set<int> preInitialisedMarkerIDs;
				for (auto marker : preInitialisedMarkers) {
					preInitialisedMarkerIDs.insert(marker->parameters.ID.get());
				}

				// Function to clean ignored markers
				auto cleanIgnoredMarkers = [](vector<shared_ptr<Markers::Marker>> markers) {
					vector<shared_ptr<Markers::Marker>> cleanedMarkers;
					for (auto marker : markers) {
						if (!marker->parameters.ignore.get()) {
							cleanedMarkers.push_back(marker);
						}
					}
					return cleanedMarkers;
				};
				cleanIgnoredMarkers(preInitialisedMarkers);


				// Gather captures with mix of initialised and uninitialised markers
				auto allCaptures = this->captures.getAllCaptures();
				int capturesAddedCount = 0;
				for (auto capture : allCaptures) {
					// Ignore selected captures (selected = already initialised in this sense)
					if (capture->isSelected()) {
						continue;
					}

					// Check if capture contains minimum number of seen markers
					size_t seenMarkersInCapture = 0;
					for (auto markerID : capture->IDs) {
						auto findMarkerInPreInitialisedSet = preInitialisedMarkerIDs.find(markerID);
						if (findMarkerInPreInitialisedSet != preInitialisedMarkerIDs.end()) {
							seenMarkersInCapture++;
						}
					}
					if (seenMarkersInCapture < this->parameters.progressiveCalibration.progressiveMarkers.minSeenMarkers.get()) {
						continue;
					}

					// Initialise the view with solvePnP against seen markers
					this->initialiseCaptureViewWithSeenMarkers(capture);

					// Select the capture
					capture->setSelected(true);

					capturesAddedCount++;

					if (capturesAddedCount >= this->parameters.progressiveCalibration.progressiveMarkers.maxCapturesToAdd.get()) {
						break;
					}
				}

				auto capturesForThisStep = this->captures.getSelection();

				// Function to solve
				auto solveWithSelectedMarkers = [this, camera](vector<shared_ptr<Markers::Marker>> markers, vector<shared_ptr<Capture>> captures) {
					// Prepare the data
					std::vector<vector<glm::vec3>> objectPoints;
					vector<Solvers::MarkerProjections::Image> images;
					Solvers::MarkerProjections::Solution initialSolution;
					vector<int> fixedObjectIndices;

					{
						for (auto marker : markers) {
							objectPoints.push_back(marker->getObjectVertices());
							initialSolution.objects.push_back(Solvers::MarkerProjections::getTransform(marker->rigidBody->getTransform()));
						}

						// gather images and view transforms
						int viewIndex = 0;
						for (auto capture : captures) {
							for (int i = 0; i < capture->IDs.size(); i++) {
								// check the markerID is in the set of markers that we are calibrating
								bool isPresent = false;
								auto markerID = capture->IDs[i];
								for (auto marker : markers) {
									if (marker->parameters.ID.get() == markerID) {
										isPresent = true;
										break;
									}
								}
								if (!isPresent) {
									// ignore this marker (which was seen in this view)
									continue;
								}

								// Serialization error previously
								if (capture->imagePointsUndistorted.empty()) {
									for (auto& imagePoints : capture->imagePoints) {
										auto undistortedImagePoints = ofxCv::undistortImagePoints(ofxCv::toCv(imagePoints)
											, camera->getCameraMatrix()
											, camera->getDistortionCoefficients());
										capture->imagePointsUndistorted.push_back(ofxCv::toOf(undistortedImagePoints));
									}
									
								}
								Solvers::MarkerProjections::Image image;
								image.imagePointsUndistorted = capture->imagePointsUndistorted[i];
								image.viewIndex = viewIndex;
								image.objectIndex = -1;
								for (int objectIndex = 0; objectIndex < markers.size(); objectIndex++) {
									if (markers[objectIndex]->parameters.ID.get() == capture->IDs[i]) {
										image.objectIndex = objectIndex;
										break;
									}
								}
								if (image.objectIndex == -1) {
									throw(ofxRulr::Exception("Marker not found when collating object indices"));
								}
								images.push_back(image);
							}

							auto transform = capture->cameraView.getGlobalTransformMatrix();
							auto viewTransform = glm::inverse(transform);
							auto transform2 = Solvers::MarkerProjections::getTransform(viewTransform);
							initialSolution.views.push_back(transform2);
							viewIndex++;
						}

						// gather fixed object indices
						for (size_t i = 0; i < markers.size(); i++) {
							auto marker = markers[i];
							if (marker->parameters.fixed.get()) {
								fixedObjectIndices.push_back(i);
							}
						}
					}

					// Perform the solve
					{
						auto solverSettings = Solvers::MarkerProjections::defaultSolverSettings();
						solverSettings.options.max_num_iterations = this->parameters.calibration.bundleAdjustment.maxIterations.get();
						solverSettings.options.function_tolerance = this->parameters.calibration.bundleAdjustment.functionTolerance.get();
						solverSettings.options.num_threads = this->parameters.calibration.bundleAdjustment.numThreads.get();

						auto cameraView = camera->getViewInObjectSpace();

						auto result = Solvers::MarkerProjections::solve(camera->getWidth()
							, camera->getHeight()
							, cameraView.getClippedProjectionMatrix()
							, objectPoints
							, images
							, fixedObjectIndices
							, initialSolution
							, solverSettings);

						if (result.residual > this->parameters.progressiveCalibration.maximumResidual.get()) {
							throw(ofxRulr::Exception("Residual is too high to continue"));
						}

						// Unpack the solution
						this->unpackSolution(captures, result.solution, markers, images);
					}
				};

				// Perform bundle adjustment with seen markers only
				{
					Utils::ScopedProcess scoppedProcessSeenMarkers("Bundle adjust with seen markers", true);
					solveWithSelectedMarkers(preInitialisedMarkers, capturesForThisStep);
					scoppedProcessSeenMarkers.end();
				}

				// Solve PnP unseen markers
				{
					Utils::ScopedProcess scopedProcessInitialiseUnseenMarkers("Initialise unseen markers");
					for (auto capture : capturesForThisStep) {
						this->initialiseUnseenMarkersInView(capture);
					}
					scopedProcessInitialiseUnseenMarkers.end();
				}

				// All markers in these captures
				vector<shared_ptr<Markers::Marker>> allMarkersInTheseCaptures;
				{
					auto allMarkers = markersNode->getMarkers(); // this will have been updated by now with unseen markers

					for (auto marker : allMarkers) {
						// check it's in a capture
						bool insideACapture = false;
						for (auto capture : capturesForThisStep) {
							for (auto markerIDInCApture : capture->IDs) {
								if (markerIDInCApture == marker->parameters.ID.get()) {
									insideACapture = true;
									break;
								}
							}
							if (insideACapture) {
								break;
							}
						}

						if (insideACapture) {
							allMarkersInTheseCaptures.push_back(marker);
						}
					}

					// Clean out unwanted markers
					allMarkersInTheseCaptures = cleanIgnoredMarkers(allMarkersInTheseCaptures);
				}
				

				// Perform bundle adjustment with the markers that we added
				{
					Utils::ScopedProcess scopedProcessBundleAdjustWithUnseenMarkers("Bundle adjust with unseen markers");
					solveWithSelectedMarkers(allMarkersInTheseCaptures, capturesForThisStep);
					scopedProcessBundleAdjustWithUnseenMarkers.end();
				}

				scopedProcess.end();

				this->dirty.capturePreviews = true;
			}
			//----------
			void Calibrate::add(const cv::Mat& image, const string& name) {
				if (image.empty()) {
					throw(ofxRulr::Exception("Image empty"));
				}
				this->throwIfMissingAnyConnection();
				auto markers = this->getInput<Markers>();
				auto camera = this->getInput<Item::Camera>();

				markers->throwIfMissingAConnection<ArUco::Detector>();
				auto detector = markers->getInput<ArUco::Detector>();

				auto foundMarkers = detector->findMarkers(image, false);
				if (foundMarkers.empty()) {
					throw(ofxRulr::Exception("No markers found"));
				}

				auto capture = make_shared<Capture>();
				capture->parent = this;
				capture->name.set(name);
				for (const auto& foundMarker : foundMarkers) {
					capture->IDs.push_back(foundMarker.id);
					auto undistortedImagePoints = ofxCv::undistortImagePoints(foundMarker
						, camera->getCameraMatrix()
						, camera->getDistortionCoefficients());
					capture->imagePointsUndistorted.push_back(ofxCv::toOf(undistortedImagePoints));
					capture->imagePoints.push_back(ofxCv::toOf((vector<cv::Point2f>&) foundMarker));
				}
				this->captures.add(capture);

				this->dirty.capturePreviews = true;
			}

			//----------
			void Calibrate::addFolderOfImages() {
				auto result = ofSystemLoadDialog("Folder of images", true);
				if (!result.bSuccess) {
					return;
				}


				ofDirectory directory(result.filePath);
				auto count = directory.listDir();

				Utils::ScopedProcess outerScopedProcess(result.filePath, false, count);

				for (const auto& file : directory) {
					auto extension = ofToLower(file.getExtension());
					bool validExtension = extension == "png"
						|| extension == "bmp"
						|| extension == "jpg"
						|| extension == "jpeg"
						|| extension == "tif"
						|| extension == "tiff";
					if (!validExtension) {
						continue;
					}

					auto path = file.getAbsolutePath();
					try {
						Utils::ScopedProcess fileScopedProcess(file.getFileName());
						auto image = cv::imread(path);
						this->add(image, ofFilePath::getBaseName(path));
						fileScopedProcess.end();
					}
					RULR_CATCH_ALL_TO_ERROR;
				}

				this->dirty.capturePreviews = true;
			}

			//----------
			void Calibrate::unpackSolution(vector<shared_ptr<Capture>> captures
				, Solvers::MarkerProjections::Solution& solution
				, vector<shared_ptr<Markers::Marker>>& markers
				, const vector<Solvers::MarkerProjections::Image>& images) {
					for (size_t i = 0; i < captures.size(); i++) {
						auto viewTransform = Solvers::MarkerProjections::getTransform(solution.views[i]);
						auto rigidBodyTransform = glm::inverse(viewTransform);

						auto rigidBodyTransform2 = Solvers::MarkerProjections::getTransform(rigidBodyTransform);

						captures[i]->cameraView.setPosition(rigidBodyTransform2.translation);

						auto orientation = ofxCeres::VectorMath::eulerToQuat(rigidBodyTransform2.rotation);
						captures[i]->cameraView.setOrientation(orientation);
					}

					for (size_t i = 0; i < markers.size(); i++) {
						auto marker = markers[i];
						marker->rigidBody->setTransform(Solvers::MarkerProjections::getTransform(solution.objects[i]));
					}

					// Unproject camera rays for image points
					for (auto capture : captures) {
						capture->cameraRays.clear();
						for (const auto& imagePointsUndistorted : capture->imagePointsUndistorted) {
							vector<ofxRay::Ray> cameraRays;
							capture->cameraView.castPixels(imagePointsUndistorted, cameraRays, false);
							for (auto& ray : cameraRays) {
								ray.color = capture->color;
								ray.t = glm::normalize(ray.t) * this->parameters.debug.cameraRayLength.get();
								capture->cameraRays.push_back(ray);
							}
						}
					}

					// Get residuals per image
					if (solution.reprojectionErrorPerImage.size() == images.size()) {
						for (auto capture : captures) {
							capture->residuals.clear();
						}
						for (size_t i = 0; i < images.size(); i++) {
							auto& image = images[i];
							captures[image.viewIndex]->residuals.push_back(solution.reprojectionErrorPerImage[i]);
						}
					}
				this->dirty.capturePreviews = true;
			};

			//----------
			void Calibrate::initialiseCaptureViewWithSeenMarkers(shared_ptr<Capture> capture) {
				this->throwIfMissingAConnection<Markers>();
				this->throwIfMissingAConnection<Item::Camera>();

				auto markerNode = this->getInput<Markers>();
				auto camera = this->getInput<Item::Camera>();

				auto size = capture->IDs.size();

				// Navigate the camera
				cv::Mat cameraRotationVector, cameraTranslation;
				{
					// Gather navigation image points and world points
					vector<cv::Point2f> imagePoints;
					vector<cv::Point3f> worldPoints;

					for (int i = 0; i < size; i++) {
						const auto& ID = capture->IDs[i];
						try {
							auto marker = markerNode->getMarkerByID(ID);

							if (marker->parameters.ignore.get()) {
								continue;
							}

							const auto& markerImagePoints = ofxCv::toCv(capture->imagePoints[i]);
							imagePoints.insert(imagePoints.end(), markerImagePoints.begin(), markerImagePoints.end());

							const auto markerWorldPoints = ofxCv::toCv(marker->getWorldVertices());
							worldPoints.insert(worldPoints.end(), markerWorldPoints.begin(), markerWorldPoints.end());
						}
						catch (...) {
							// marker is not initialised
						}
					}

					if (imagePoints.empty()) {
						throw(ofxRulr::Exception("Cannot initialise view - no prior markers found"));
					}

					cv::solvePnP(worldPoints
						, imagePoints
						, camera->getCameraMatrix()
						, camera->getDistortionCoefficients()
						, cameraRotationVector
						, cameraTranslation);

					camera->setExtrinsics(cameraRotationVector
						, cameraTranslation
						, true);
					capture->cameraView = camera->getViewInWorldSpace();
					capture->cameraView.setFarClip(this->parameters.debug.cameraFarPlane.get());
					capture->cameraView.color = capture->color;
					capture->initialised = true;
				}

				this->dirty.capturePreviews = true;
			}

			//----------
			void Calibrate::calibrateProgressiveMarkersContinuously() {
				Utils::ScopedProcess scopedProcess("Calibrate Progressive Markers continuously");
				auto allCaptureCount = this->captures.getAllCapturesUntyped().size();
				int lastActiveCaptureCount = this->captures.getSelection().size();
				for (int i = 0; i < this->parameters.progressiveCalibration.progressiveMarkers.maxTriesContinuous.get(); i++) {
					this->calibrateProgressiveMarkers();
					auto newActiveCaptureCount = this->captures.getSelection().size();
					if (newActiveCaptureCount == lastActiveCaptureCount) {
						// All captures initialised
						break;
					}
					lastActiveCaptureCount = newActiveCaptureCount;
				}
				scopedProcess.end();
				this->dirty.capturePreviews = true;
			}

			//----------
			void Calibrate::initialiseUnseenMarkersInView(shared_ptr<Capture> capture) {
				this->throwIfMissingAConnection<Markers>();
				this->throwIfMissingAConnection<Item::Camera>();

				auto markersNode = this->getInput<Markers>();
				markersNode->throwIfMissingAnyConnection();
				auto detector = markersNode->getInput<ArUco::Detector>();
				auto markerLength = detector->getMarkerLength();

				auto camera = this->getInput<Item::Camera>();

				for (int i = 0; i < capture->IDs.size(); i++) {
					auto markerID = capture->IDs[i];

					try {
						markersNode->getMarkerByID(markerID);
						continue;
					}
					catch (...) {
						// it's missing - which is good in this case
					}

					auto marker = make_shared<Markers::Marker>();
					marker->parameters.ID.set(markerID); 
					marker->parameters.length.set(markerLength);

					// Note that this function sets length and parent
					markersNode->add(marker);

					const auto& imagePoints = ofxCv::toCv(capture->imagePoints[i]);
					const auto objectPoints = ofxCv::toCv(marker->getObjectVertices());

					cv::Mat markerRotationVector, markerTranslation;
					cv::solvePnP(objectPoints
						, imagePoints
						, camera->getCameraMatrix()
						, camera->getDistortionCoefficients()
						, markerRotationVector
						, markerTranslation);

					marker->rigidBody->setExtrinsics(markerRotationVector
						, markerTranslation
						, false);

					// And apply the camera transform on top
					auto transform = capture->cameraView.getLocalTransformMatrix() * marker->rigidBody->getTransform();
					marker->rigidBody->setTransform(transform);
				}

				this->dirty.capturePreviews = true;
			}

			//----------
			void Calibrate::updateCapturePreviews()
			{
				auto captures = this->captures.getAllCaptures();

				// Calculate the max residual
				float maxResidual = 0.01;
				for (auto capture : captures) {
					for (auto residual : capture->residuals) {
						if (residual > maxResidual) {
							maxResidual = residual;
						}
					}
				}

				// Get room min and room max
				auto worldStage = ofxRulr::Graph::World::X().getWorldStage();
				auto worldStagePanel = worldStage->getPanelTyped();
				auto roomMin = worldStagePanel->parameters.grid.roomMin.get();
				auto roomMax = worldStagePanel->parameters.grid.roomMax.get();

				// Update the captures
				Capture::UpdateArgs args{
					maxResidual
					, roomMin
					, roomMax
					, true
				};
				for (auto capture : captures) {
					capture->update(args);
				}

				this->dirty.capturePreviews = false;
			}
		}
	}
}
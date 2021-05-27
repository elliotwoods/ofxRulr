#include "pch_Plugin_ArUco.h"
#include "ofxRulr/Solvers/MarkerProjections.h"

namespace ofxRulr {
	namespace Nodes {
		namespace MarkerMap {
#pragma mark Capture
			//----------
			Calibrate::Capture::Capture() {
				RULR_SERIALIZE_LISTENERS;
			}

			//----------
			string Calibrate::Capture::getDisplayString() const {
				stringstream ss;
				for (const auto& ID : this->IDs) {
					ss << ID << " ";
				}
				return ss.str();
			}

			//----------
			void Calibrate::Capture::drawWorld() {

			}

			//----------
			void Calibrate::Capture::serialize(nlohmann::json& json) {
				Utils::serialize(json, "IDs", this->IDs);
				Utils::serialize(json, "imagePoints", this->imagePoints);
			}

			//----------
			void Calibrate::Capture::deserialize(const nlohmann::json& json) {
				Utils::deserialize(json, "IDs", this->IDs);
				Utils::deserialize(json, "imagePoints", this->imagePoints);
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
				}

				this->addInput<Item::Camera>();
				this->addInput<Markers>();
			}

			//----------
			void Calibrate::update() {

			}

			//----------
			void Calibrate::drawWorldStage() {
				auto captures = this->captures.getSelection();
				for (auto capture : captures) {
					if (this->parameters.draw.cameraRays.get()) {
						for (const auto& cameraRay : capture->cameraRays) {
							cameraRay.draw();
						}
					}
					if (this->parameters.draw.cameraViews.get()) {
						capture->cameraView.draw();
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
				inspector->addButton("Calibrate", [this]() {
					try {
						Utils::ScopedProcess scopedProcess("Calibrate");
						this->calibrate();
						scopedProcess.end();
					}
					RULR_CATCH_ALL_TO_ALERT;
					}, OF_KEY_RETURN)->setHeight(100.0f);
			}

			//----------
			void Calibrate::serialize(nlohmann::json& json) {
				this->captures.serialize(json["captures"]);
			}

			//----------
			void Calibrate::deserialize(const nlohmann::json& json) {
				if (json.contains("captures")) {
					this->captures.deserialize(json["captures"]);
				}
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
				this->add(image);
			}

			//----------
			void Calibrate::calibrate() {
				this->throwIfMissingAnyConnection();
				auto camera = this->getInput<Item::Camera>();
				auto markersNode = this->getInput<Markers>();

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

						cv::UsacParams usacParams;
						{
							// 2 px reprojection error
							usacParams.threshold = 2;
						}

						std::vector<int> inliers;
						cv::solvePnPRansac(worldPoints
							, imagePoints
							, camera->getCameraMatrix()
							, camera->getDistortionCoefficients()
							, cameraRotationVector
							, cameraTranslation
							, inliers
							, usacParams);

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
				{
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

							cv::UsacParams usacParams;
							{
								// 2 px reprojection error
								usacParams.threshold = 2;
							}

							std::vector<int> inliers;
							cv::solvePnPRansac(worldPoints
								, imagePoints
								, camera->getCameraMatrix()
								, camera->getDistortionCoefficients()
								, cameraRotationVector
								, cameraTranslation
								, inliers
								, usacParams);

							camera->setExtrinsics(cameraRotationVector
								, cameraTranslation
								, true);
							capture->cameraView = camera->getViewInWorldSpace();
							capture->cameraView.setFarClip(this->parameters.debug.cameraFarPlane.get());
						}
					}
				}

				// Perform the bundle adjustment solve
				vector<int> markerIDs;
				{
					std::vector<vector<glm::vec3>> objectPoints;
					vector<Solvers::MarkerProjections::Image> images;
					Solvers::MarkerProjections::Solution initialSolution;
					vector<int> fixedObjectIndices;

					// Gather data
					{
						// gather marker IDs
						{
							for (auto marker : initialisationMarkers) {
								markerIDs.push_back(marker->parameters.ID.get());
							}
							for (auto marker : markersWeInitialised) {
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
							for (size_t i = 0; i < size; i++) {
								Solvers::MarkerProjections::Image image;
								image.imagePoints = capture->imagePoints[i];
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
							auto transform3 = Solvers::MarkerProjections::getTransform(transform2);
							auto transform4 = Solvers::MarkerProjections::getTransform(transform3);
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

					// Perform solve
					{
						// Unpack result
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
								for (const auto& imagePoints : capture->imagePoints) {
									vector<ofxRay::Ray> cameraRays;
									capture->cameraView.castPixels(imagePoints, cameraRays, false);
									for (auto& ray : cameraRays) {
										ray.color = capture->color;
										ray.t = glm::normalize(ray.t) * this->parameters.debug.cameraRayLength.get();
										capture->cameraRays.push_back(ray);
									}
								}

							}
						};

						// Perform solve
						if (this->parameters.calibration.bundleAdjustment.enabled.get()) {
							auto solverSettings = Solvers::MarkerProjections::defaultSolverSettings();
							solverSettings.options.max_num_iterations = this->parameters.calibration.bundleAdjustment.maxIterations.get();
							solverSettings.options.function_tolerance = this->parameters.calibration.bundleAdjustment.functionTolerance.get();

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
			}

			//----------
			void Calibrate::add(const cv::Mat& image) {
				if (image.empty()) {
					throw(ofxRulr::Exception("Image empty"));
				}
				this->throwIfMissingAnyConnection();
				auto markers = this->getInput<Markers>();
				auto camera = this->getInput<Item::Camera>();

				markers->throwIfMissingAConnection<ArUco::Detector>();
				auto detector = markers->getInput<ArUco::Detector>();

				auto foundMarkers = detector->findMarkers(image);
				if (foundMarkers.empty()) {
					throw(ofxRulr::Exception("No markers found"));
				}

				auto capture = make_shared<Capture>();
				for (const auto& foundMarker : foundMarkers) {
					capture->IDs.push_back(foundMarker.id);
					auto undistortedImagePoints = ofxCv::undistortImagePoints(foundMarker
						, camera->getCameraMatrix()
						, camera->getDistortionCoefficients());
					capture->imagePoints.push_back(ofxCv::toOf(undistortedImagePoints));
				}
				this->captures.add(capture);
			}

			//----------
			void Calibrate::addFolderOfImages() {
				auto result = ofSystemLoadDialog("Folder of images", true);
				if (!result.bSuccess) {
					return;
				}

				ofDirectory directory(result.filePath);
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
					ofLogNotice("MarkerMap::Calibrate") << "Loading " << path;
					auto image = cv::imread(path);
					try {
						this->add(image);
					}
					RULR_CATCH_ALL_TO_ERROR;
				}
			}
		}
	}
}
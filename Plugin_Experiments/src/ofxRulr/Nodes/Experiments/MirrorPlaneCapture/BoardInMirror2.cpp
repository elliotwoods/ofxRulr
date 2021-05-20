#include "pch_Plugin_Experiments.h"
#include "ofxRulr/Nodes/Procedure/Calibrate/ExtrinsicsFromBoardInWorld.h"

namespace ofxRulr {
	namespace Nodes {
		namespace Experiments {
			namespace MirrorPlaneCapture {
#pragma mark PhotoCapture

				//--------
				BoardInMirror2::Capture::Capture() {
					RULR_SERIALIZE_LISTENERS;
				}

				//--------
				void BoardInMirror2::Capture::drawWorld() {
					ofPushStyle();
					{
						ofSetColor(this->color);
						ofSetSphereResolution(6);
						for (const auto& worldPoint : this->worldPoints) {
							ofDrawSphere(ofxCv::toOf(worldPoint), 0.01);
						}
					}
					ofPopStyle();

					for (const auto& ray : this->cameraRays) {
						ray.draw();
					}
				}

				//--------
				string BoardInMirror2::Capture::getDisplayString() const {
					stringstream ss;
					ss << this->worldPoints.size() << " points found.";
					return ss.str();
				}

				//--------
				void BoardInMirror2::Capture::serialize(nlohmann::json& json) {
					Utils::serialize(json["imagePoints"], this->imagePoints);
					Utils::serialize(json["worldPoints"], this->worldPoints);
					Utils::serialize(json["cameraRays"], this->cameraRays);
					Utils::serialize(json["cameraPosition"], this->cameraPosition);
				}

				//--------
				void BoardInMirror2::Capture::deserialize(const nlohmann::json& json) {
					Utils::deserialize(json, "imagePoints", this->imagePoints);
					Utils::deserialize(json, "worldPoints", this->worldPoints);
					Utils::deserialize(json, "cameraRays", this->cameraRays);
					Utils::deserialize(json, "cameraPosition", this->cameraPosition);
				}

#pragma mark BoardInMirror2

				//--------
				BoardInMirror2::BoardInMirror2() {
					RULR_NODE_INIT_LISTENER;
				}

				//--------
				string BoardInMirror2::getTypeName() const {
					return "Halo::BoardInMirror2";
				}

				//--------
				void BoardInMirror2::init() {
					RULR_NODE_UPDATE_LISTENER;
					RULR_NODE_SERIALIZATION_LISTENERS;
					RULR_NODE_INSPECTOR_LISTENER;
					RULR_NODE_DRAW_WORLD_LISTENER;

					this->manageParameters(this->parameters);

					this->addInput<Heliostats2>();
					this->addInput<Item::Camera>();
					this->addInput<ArUco::MarkerMapPoseTracker>();
					this->addInput<Procedure::Calibrate::ExtrinsicsFromBoardInWorld>();
				}

				//--------
				void BoardInMirror2::update() {
					{
						bool tetheredEnabled = false;
						switch (this->parameters.tetheredShootEnabled.get()) {
						case WhenDrawOnWorldStage::Selected:
							tetheredEnabled = this->isBeingInspected();
							break;
						case WhenDrawOnWorldStage::Always:
							tetheredEnabled = true;
						default:
							break;
						}
						if (tetheredEnabled) {
							auto camera = this->getInput<Item::Camera>();
							if (camera) {
								if (camera->isNewSingleShotFrame()) {
									try {
										Utils::ScopedProcess scopedProcess("Tethered capture");
										this->addCapture(camera->getFrame());
										scopedProcess.end();
									}
									RULR_CATCH_ALL_TO_ERROR
								}
							}
						}
					}
				}

				//--------
				void BoardInMirror2::capture() {
					Utils::ScopedProcess scopedProcess("Capture");
					this->throwIfMissingAConnection<Item::Camera>();
					auto camera = this->getInput<Item::Camera>();
					auto frame = camera->getFreshFrame();
					scopedProcess.end();

					this->addCapture(frame);
				}


				//--------
				void BoardInMirror2::addCapture(shared_ptr<ofxMachineVision::Frame> frame) {
					if (!frame) {
						throw(Exception("Frame is empty"));
					}

					// Get inputs
					this->throwIfMissingAnyConnection();
					auto camera = this->getInput<Item::Camera>();
					auto heliostats = this->getInput<Heliostats2>();
					auto extrinsicsFromBoardInWorld = this->getInput<Procedure::Calibrate::ExtrinsicsFromBoardInWorld>();
					extrinsicsFromBoardInWorld->throwIfMissingAnyConnection();
					auto boardInWorld = extrinsicsFromBoardInWorld->getInput<Item::BoardInWorld>();
					heliostats->throwIfMissingAConnection<Dispatcher>();
					auto dispatcher = heliostats->getInput<Dispatcher>();

					// Build solver settings
					auto solverSettings = this->getSolverSettings();

					// Face all the modules away (async)
					{
						heliostats->faceAllAway();
					}
					auto waitForTurnAway = std::async(std::launch::async, [&]() {
						heliostats->update();
						heliostats->pushStale(true);
						});

					// Note that this image is not used here. MarkerMapPoseTracker gets the image direct from its own input
					auto navigationImage = ofxCv::toCv(frame->getPixels());

					// Update the camera pose
					{

						Utils::ScopedProcess scopedProcess("Update marker map tracking", false);
						auto markerMapPoseTracker = this->getInput<ArUco::MarkerMapPoseTracker>();
						markerMapPoseTracker->track();
						scopedProcess.end();
					}

					// Update the BoardInWorld extrinsics (take a fresh frame for this)
					{
						Utils::ScopedProcess scopedProcess("Wait for motion + Capture board extrinsics", false);

						// this needs to be done after the mirror points away from the board
						waitForTurnAway.wait();

						// This takes a fresh photo from the camera again
						extrinsicsFromBoardInWorld->track(Procedure::Calibrate::ExtrinsicsFromBoardInWorld::UpdateTarget::Board, true);
					}

					// Gather heliostats in view. 
					vector<shared_ptr<Heliostats2::Heliostat>> activeHeliostats;
					{
						Utils::ScopedProcess scopedProcess("Gather heliostats in view", false);

						auto selectedHeliostats = heliostats->getHeliostats();
						auto cameraWorldView = camera->getViewInWorldSpace();
						for (auto heliostat : selectedHeliostats) {
							// project heliostat into camera
							auto heliostatInCamera = cameraWorldView.getNormalizedSCoordinateOfWorldPosition(heliostat->parameters.hamParameters.position.get());
							if (heliostatInCamera.x < -1.0f
								|| heliostatInCamera.x > 1.0f
								|| heliostatInCamera.y < -1.0f
								|| heliostatInCamera.y > 1.0f
								|| heliostatInCamera.z < 0.0f) {
								// ignore
							}
							else {
								// inside view - collect it
								activeHeliostats.push_back(heliostat);
							}
						}
					}

					// Point the active heliostats towards the board
					{
						Utils::ScopedProcess scopedProcess("Point active heliostats towards board", false);

						// Get the center of the board in world space
						glm::vec3 boardCenter(0.0f, 0.0f, 0.0f);
						{
							auto worldPoints = boardInWorld->getAllWorldPoints();
							for (const auto& worldPoint : worldPoints) {
								boardCenter += worldPoint;
							}
							boardCenter /= (float)worldPoints.size();
						}

						for (auto heliostat : activeHeliostats) {
							heliostat->navigateToReflectPointToPoint(camera->getPosition()
								, boardCenter
								, solverSettings);
						}

						heliostats->update();
						heliostats->pushStale(true);
					}

					// Take a fresh photo (now the photo contains the reflected boards and the original board)
					cv::Mat imageWithReflections;
					{
						Utils::ScopedProcess scopedProcess("Take a photo of board reflections", false);

						ofSleepMillis(this->parameters.servoControl.waitTime * 1000.0f);
						auto frame = camera->getFreshFrame();

						if (!frame) {
							throw(ofxRulr::Exception("Failed to get capture"));
						}

						// Extract the image
						imageWithReflections = ofxCv::toCv(frame->getPixels()).clone();
						{
							if (imageWithReflections.empty()) {
								throw(ofxRulr::Exception("No image in capture"));
							}
						}
					}
					// Record all servo Present Position values at this time
					map<Dispatcher::ServoID, Dispatcher::RegisterValue> servoValues;
					{
						auto servoIDs = dispatcher->getServoIDs();

						Dispatcher::MultiGetRequest multiGetRequest;
						{
							multiGetRequest.servoIDs = servoIDs;
							multiGetRequest.registerName = "Present Position";
						}
						auto servoValueVector = dispatcher->multiGetRequest(multiGetRequest);
						for (int i = 0; i < servoIDs.size(); i++) {
							servoValues[servoIDs[i]] = servoValueVector[i];
						}
					}

					// Mask out the direct view of the board
					cv::Mat imageWithReflectionsWithoutRealBoard = imageWithReflections.clone();
					auto cameraView = camera->getViewInWorldSpace();
					{
						Utils::ScopedProcess scopedProcess("Mask out the real board", false);

						// Allocate an fbo
						ofFbo fbo;
						{
							ofFbo::Settings fboSettings;
							{
								fboSettings.width = cameraView.getWidth();
								fboSettings.height = cameraView.getHeight();
								fboSettings.internalformat = GL_RGBA;
								fboSettings.useDepth = true;
								fboSettings.depthStencilInternalFormat = GL_DEPTH_COMPONENT24;
								fboSettings.minFilter = GL_NEAREST;
								fboSettings.maxFilter = GL_NEAREST;
								fboSettings.numSamples = 0;
							}
							fbo.allocate(fboSettings);
						}

						fbo.begin();
						{
							ofClear(0, 0);
							cameraView.beginAsCamera();
							{
								ofEnableDepthTest();
								boardInWorld->drawWorldStage();
								ofDisableDepthTest();
							}
							cameraView.endAsCamera();
						}
						fbo.end();

						// read back the fbo
						ofPixels pixels;
						fbo.readToPixels(pixels);
						
						// make a mask from the alpha channel
						cv::Mat rgbaImage = ofxCv::toCv(pixels);
						cv::Mat alphaMask;
						cv::extractChannel(rgbaImage, alphaMask, 3);
						cv::flip(alphaMask
							, alphaMask
							, 0);
						imageWithReflectionsWithoutRealBoard.setTo(cv::Scalar(0), alphaMask);
					}

					// Find boards in each heliostat mirror face
					{
						Utils::ScopedProcess scopedProcess("Find reflected boards in mirrors", false, activeHeliostats.size());

						for (auto heliostat : activeHeliostats) {
							try {
								Utils::ScopedProcess heliostatScopedProcess("Heliostat : " + heliostat->getName());
								this->processHeliostat(heliostats
									, heliostat
									, cameraView
									, imageWithReflectionsWithoutRealBoard
									, boardInWorld
									, servoValues);
								heliostatScopedProcess.end();
							}
							RULR_CATCH_ALL_TO_ERROR;
						}
					}
				}

				//--------
				void BoardInMirror2::calibrate() {
					// Gather data
					vector<ofxRay::Ray> cameraRays;
					vector<glm::vec3> worldPoints;
					{
						auto captures = this->captures.getSelection();
						for (const auto& capture : captures) {
							cameraRays.insert(cameraRays.end(), capture->cameraRays.begin(), capture->cameraRays.end());

							const auto& worldPointsToAdd = ofxCv::toOf(capture->worldPoints);
							worldPoints.insert(worldPoints.end(), worldPointsToAdd.begin(), worldPointsToAdd.end());
						}
					}

					// Normalise rays
					{
						for (auto& cameraRay : cameraRays) {
							cameraRay.t = glm::normalize(cameraRay.t);
						}
					}


					// Run solve
					{
						auto result = ofxRulr::Solvers::MirrorPlaneFromRays::solve(cameraRays
							, worldPoints
							, this->getSolverSettings());

						this->result = make_shared<ofxRulr::Solvers::MirrorPlaneFromRays::Result>(result);
					}

					// Make the debug draw objects
					{
						auto abcd = this->result->solution.plane.getABCD();
						glm::vec3 planeNormal{ abcd.x, abcd.y, abcd.z };
						float planeD = abcd.w;

						this->debug.reflectedRays.clear();
						for (const auto& cameraRay : cameraRays) {
							// intersect the ray and the plane
							const auto u = -(planeD + glm::dot(cameraRay.s, planeNormal))
								/ glm::dot(cameraRay.t, planeNormal);
							auto intersection = cameraRay.s + u * cameraRay.t;

							// reflected ray starts with intersection between cameraRay and mirror plane
							const auto& reflectedRayS = intersection;

							// reflected ray transmission is defined by reflect(cameraRayT + reflectedRayS, mirrorPlane)
							auto reflectedRayT = ofxCeres::VectorMath::reflect(cameraRay.t + reflectedRayS, planeNormal, planeD) - reflectedRayS;

							this->debug.reflectedRays.push_back({ reflectedRayS, reflectedRayT });
						}
					}
				}

				//--------
				ofxCvGui::PanelPtr BoardInMirror2::getPanel() {
					return this->panel;
				}

				//--------
				void BoardInMirror2::populateInspector(ofxCvGui::InspectArguments& inspectArgs) {
					auto inspector = inspectArgs.inspector;
					this->captures.populateWidgets(inspector);
					inspector->addButton("Capture", [this]() {
						try {
							this->capture();
						}
						RULR_CATCH_ALL_TO_ALERT
						}, ' ');
					inspector->addButton("Calibrate", [this]() {
						try {
							this->calibrate();
						}
						RULR_CATCH_ALL_TO_ALERT
						}, OF_KEY_RETURN)->setHeight(100.0f);

						inspector->addIndicatorBool("Converged", [this]() {
							if (this->result) {
								return this->result->isConverged();
							}
							else {
								return false;
							}
							});
						inspector->addLiveValue<double>("Residual [m]", [this]() {
							if (this->result) {
								return this->result->residual;
							}
							else {
								return 0.0;
							}
							});
				}

				//--------
				void BoardInMirror2::serialize(nlohmann::json& json) {
					this->captures.serialize(json["captures"]);
				}

				//--------
				void BoardInMirror2::deserialize(const nlohmann::json& json) {
					if (json.contains("captures")) {
						this->captures.deserialize(json["captures"]);
					}
				}

				//--------
				void BoardInMirror2::drawWorldStage() {
					// Draw data
					{
						auto captures = this->captures.getSelection();
						for (const auto& capture : captures) {
							capture->drawWorld();
						}
					}

					// Draw result
					if (this->result) {
						this->result->solution.plane.draw();
					}

					// Draw debug info
					if (this->parameters.debug.draw.reflectedRays) {
						for (const auto& ray : this->debug.reflectedRays) {
							ray.draw();
						}
					}
				}

				//----------
				ofxCeres::SolverSettings BoardInMirror2::getSolverSettings() const {
					auto solverSettings = ofxRulr::Solvers::HeliostatActionModel::Navigator::defaultSolverSettings();
					
					solverSettings.options.max_num_iterations = this->parameters.solve.maxIterations.get();
					solverSettings.printReport = this->parameters.solve.printReport.get();
					solverSettings.options.minimizer_progress_to_stdout = this->parameters.solve.printReport.get();

					return solverSettings;
				}

				//----------
				void BoardInMirror2::processHeliostat(shared_ptr<Heliostats2> heliostats
					, shared_ptr<Heliostats2::Heliostat> heliostat
					, const ofxRay::Camera& cameraView
					, const cv::Mat& imageWithReflectionsWithoutRealBoard
					, shared_ptr<Item::BoardInWorld> boardInWorld
					, const map<Dispatcher::ServoID, Dispatcher::RegisterValue>& servoValues)
				{
					auto dispatcher = heliostats->getInput<Dispatcher>();

					// Get the mask for this mirror
					auto mask = heliostats->drawMirrorFaceMask(heliostat, cameraView);

					// Get the bounding box of the mask
					cv::Rect boundingBox;
					{
						cv::Mat activePixels;
						cv::findNonZero(mask, activePixels);
						boundingBox = cv::boundingRect(activePixels);
					}

					// Mask the image
					cv::Mat maskedImage = imageWithReflectionsWithoutRealBoard.clone();
					{
						cv::Mat invertedMask;
						cv::bitwise_not(mask, invertedMask);
						maskedImage.setTo(cv::Scalar(0), invertedMask);
					}

					// Find the board in the masked image
					auto capture = make_shared<Capture>();
					{
						vector<cv::Point2f> imagePointsCropped;
						vector<cv::Point3f> objectPoints;

						auto croppedRegion = maskedImage(boundingBox);

						cv::normalize(croppedRegion
							, croppedRegion
							, 255
							, 0
							, cv::NORM_INF);

						// Find the board in the cropped image
						{
							bool foundBoard = boardInWorld->findBoard(croppedRegion
								, imagePointsCropped
								, objectPoints
								, capture->worldPoints
								, this->parameters.findBoard.mode.get());

							if (!foundBoard && this->parameters.findBoard.useAssistantIfFail.get()) {
								foundBoard = boardInWorld->findBoard(croppedRegion
									, imagePointsCropped
									, objectPoints
									, capture->worldPoints
									, FindBoardMode::Assistant);
							}
							if (!foundBoard) {
								throw(Exception("Failed to find board in reflection"));
							}
						}

						// Move the image points back into uncropped space
						capture->imagePoints.reserve(imagePointsCropped.size());
						cv::Point2f offset(boundingBox.x, boundingBox.y);
						for (const auto& imagePointCropped : imagePointsCropped) {
							capture->imagePoints.push_back(imagePointCropped + offset);
						}
					}

					// Calculate camera rays associated with these image points / world points
					{
						// unproject rays
						cameraView.castPixels(ofxCv::toOf(capture->imagePoints)
							, capture->cameraRays
							, true);

						// set the colors
						for (auto& ray : capture->cameraRays) {
							ray.color = capture->color;
						}
					}

					capture->cameraPosition = cameraView.getPosition();
					capture->heliostatName = heliostat->getName();

					// Find axis servo values
					auto findServoValue = [&](const int& servoID) {
						auto findServoValue = servoValues.find(servoID);
						if (findServoValue == servoValues.end()) {
							throw(ofxRulr::Exception("Failed to get a Present Position for Servo [" + ofToString(servoID) + "]"));
						}
						return findServoValue->second;
					};
					capture->axis1ServoPosition = findServoValue(heliostat->parameters.servo1.ID.get());
					capture->axis2ServoPosition = findServoValue(heliostat->parameters.servo2.ID.get());


					// crop the camera rays to intersect the current mirror
					{
						// Get the plane for the mirror
						glm::vec3 mirrorCenter, mirrorNormal;
						Solvers::HeliostatActionModel::getMirrorCenterAndNormal<float>({
							heliostat->parameters.servo1.angle.get()
							, heliostat->parameters.servo2.angle.get()
							}, heliostat->getHeliostatActionModelParameters()
							, mirrorCenter
							, mirrorNormal);
						ofxRay::Plane plane(mirrorCenter, mirrorNormal);

						// Individually crop the rays
						for (auto& cameraRay : capture->cameraRays) {
							glm::vec3 position;
							if (plane.intersect(cameraRay, position)) {
								cameraRay.setEnd(position);
							}
							cameraRay.infinite = false;
						}
					}

					this->captures.add(capture);
				}
			}
		}
	}
}
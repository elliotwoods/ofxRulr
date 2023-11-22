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
				void BoardInMirror2::Capture::drawWorld(const DrawOptions& drawOptions) {
					if (drawOptions.camera) {
						this->camera.draw();
					}

					if (drawOptions.cameraRays) {
						for (const auto& ray : this->cameraRays) {
							ray.draw();
						}
					}

					if (drawOptions.worldPoints) {
						ofPushStyle();
						{
							ofSetColor(this->color);
							ofSetSphereResolution(6);
							for (const auto& worldPoint : this->worldPoints) {
								ofDrawSphere(ofxCv::toOf(worldPoint), drawOptions.worldPointsSize);
							}
						}
						ofPopStyle();
					}

					if (drawOptions.labelWorldPoints) {
						glm::vec3 mean{ 0, 0, 0 };
						for (const auto& worldPoint : this->worldPoints) {
							mean += ofxCv::toOf(worldPoint);
						}
						mean /= this->worldPoints.size();

						ofxCvGui::Utils::drawTextAnnotation(this->timeString
							, mean
							, this->color);
					}

					if (drawOptions.reflectedRays) {
						for (const auto& ray : this->reflectedRays) {
							ray.draw();
						}
					}
				
					if (drawOptions.mirrorFace) {
						ofPushStyle();
						{
							ofSetColor(this->color);
							Solvers::HeliostatActionModel::drawMirror(this->mirrorCenter
								, this->mirrorNormal
								, this->mirrorDiameter
							);
						}
						ofPopStyle();
					}
				}

				//--------
				string BoardInMirror2::Capture::getDisplayString() const {
					stringstream ss;
					ss << "[" << this->heliostatName << "] ";
					ss << "Size : " << this->worldPoints.size() << ". r=" << this->meanResidual << std::endl
						<< this->comments;

					return ss.str();
				}

				//--------
				void BoardInMirror2::Capture::serialize(nlohmann::json& json) {
					Utils::serialize(json, "imagePoints", this->imagePoints);
					Utils::serialize(json, "worldPoints", this->worldPoints);
					Utils::serialize(json, "camera", this->camera);
					Utils::serialize(json, "cameraRays", this->cameraRays);
					Utils::serialize(json, "cameraPosition", this->cameraPosition);
					Utils::serialize(json, "heliostatName", this->heliostatName);
					Utils::serialize(json, "axis1ServoPosition", this->axis1ServoPosition);
					Utils::serialize(json, "axis2ServoPosition", this->axis2ServoPosition);
					Utils::serialize(json, "residuals", this->residuals);
					Utils::serialize(json, "meanResidual", this->meanResidual);
					Utils::serialize(json, "comments", this->comments);
					Utils::serialize(json, "reflectedRays", this->reflectedRays);
					Utils::serialize(json, "mirrorCenter", this->mirrorCenter);
					Utils::serialize(json, "mirrorNormal", this->mirrorNormal);
				}

				//--------
				void BoardInMirror2::Capture::deserialize(const nlohmann::json& json) {
					Utils::deserialize(json, "imagePoints", this->imagePoints);
					Utils::deserialize(json, "worldPoints", this->worldPoints);
					Utils::deserialize(json, "camera", this->camera);
					Utils::deserialize(json, "cameraRays", this->cameraRays);
					Utils::deserialize(json, "cameraPosition", this->cameraPosition);
					Utils::deserialize(json, "heliostatName", this->heliostatName);
					Utils::deserialize(json, "axis1ServoPosition", this->axis1ServoPosition);
					Utils::deserialize(json, "axis2ServoPosition", this->axis2ServoPosition);
					Utils::deserialize(json, "residuals", this->residuals);
					Utils::deserialize(json, "meanResidual", this->meanResidual);
					Utils::deserialize(json, "comments", this->comments);
					Utils::deserialize(json, "reflectedRays", this->reflectedRays);
					Utils::deserialize(json, "mirrorCenter", this->mirrorCenter);
					Utils::deserialize(json, "mirrorNormal", this->mirrorNormal);
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
					this->addInput<MarkerMap::NavigateCamera>();
					this->addInput<Procedure::Calibrate::ExtrinsicsFromBoardInWorld>();

					{
						this->panel = ofxCvGui::Panels::makeWidgets();
						this->captures.populateWidgets(this->panel);
						this->panel->addButton("Filter to heliostat selection", [this]() {
							try {
								this->filterCapturesToHeliostatSelection();
							}
							RULR_CATCH_ALL_TO_ALERT;
							});

						this->panel->addButton("Sort by Heliostat", [this]() {
							try {
								this->captures.sortBy([](shared_ptr<Capture> capture) {
									return ofToFloat(capture->heliostatName);
									});
							}
							RULR_CATCH_ALL_TO_ALERT;
							});

						this->panel->addButton("Sort by date", [this]() {
							try {
								this->captures.sortByDate();
							}
							RULR_CATCH_ALL_TO_ALERT;
							});

						this->panel->addButton("Filter selection to inside camera", [this]() {
							try {
								this->filterHeliostatsToInsideCamera();
							}
							RULR_CATCH_ALL_TO_ALERT;
							});
					}
				}

				//--------
				void BoardInMirror2::update() {
					{
						bool tetheredEnabled = isActive(this, this->parameters.tetheredShootEnabled.get());
						if (tetheredEnabled) {
							auto camera = this->getInput<Item::Camera>();
							if (camera) {
								if (camera->isNewSingleShotFrame()) {
									try {
										Utils::ScopedProcess scopedProcess("Tethered capture");
										// set fresh frame to false for the camera
										camera->update();
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

					auto heliostats = this->getInput<Heliostats2>();

					try {

						// Get inputs
						this->throwIfMissingAnyConnection();
						auto camera = this->getInput<Item::Camera>();
						auto extrinsicsFromBoardInWorld = this->getInput<Procedure::Calibrate::ExtrinsicsFromBoardInWorld>();
						extrinsicsFromBoardInWorld->throwIfMissingAnyConnection();
						auto boardInWorld = extrinsicsFromBoardInWorld->getInput<Item::BoardInWorld>();
						heliostats->throwIfMissingAConnection<Dispatcher>();
						auto dispatcher = heliostats->getInput<Dispatcher>();

						// Build navigate solver settings
						auto navigateSolverSettings = this->getNavigateSolverSettings();

						// Turn modules away ready for board capture. We don't turn away if findBoard is disabled
						std::future<void> waitForTurnAway;
						if (this->parameters.faceAway.beforeFindBoardDirect.get() && this->parameters.findBoard.atStart.get()) {
							heliostats->faceAllAway();

							if (this->parameters.faceAway.waitAtStart.get()) {
								waitForTurnAway = std::async(std::launch::async, [&]() {
									heliostats->update();
									heliostats->pushStale(true, true);
									});
							}
						}

						// Pull the axis limits every time just in case
						heliostats->pullAllLimits();

						auto navigationImage = ofxCv::toCv(frame->getPixels());

						// Update the camera pose
						if(this->parameters.cameraNavigation.enabled.get()) {
							Utils::ScopedProcess scopedProcess("MarkerMap::NavigateCamera track", false);
							auto markerMapPoseTracker = this->getInput<MarkerMap::NavigateCamera>();
							markerMapPoseTracker->track(navigationImage, this->parameters.cameraNavigation.trustPriorPose.get());
						}

						// Update the BoardInWorld extrinsics (take a fresh frame for this)
						if (this->parameters.findBoard.atStart.get()) {
							if (waitForTurnAway.valid()) {
								Utils::ScopedProcess scopedProcess("Wait for motion + Capture board extrinsics", false);

								// this needs to be done after the mirror points away from the board
								waitForTurnAway.wait();

								// This takes a fresh photo from the camera again
								extrinsicsFromBoardInWorld->track(Procedure::Calibrate::ExtrinsicsFromBoardInWorld::UpdateTarget::Board, true);
							}
							else {
								Utils::ScopedProcess scopedProcess("Find board extrinsics", false);

								// Use existing photo to find board extrinsics
								extrinsicsFromBoardInWorld->track(Procedure::Calibrate::ExtrinsicsFromBoardInWorld::UpdateTarget::Board, false);
							}
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

						// Get the camera view
						auto cameraView = camera->getViewInWorldSpace();

						// Perform captures
						this->navigateToSeeBoardAndCapture(activeHeliostats
							, heliostats
							, extrinsicsFromBoardInWorld
							, navigateSolverSettings
							, boardInWorld
							, camera
							, cameraView
							, dispatcher
							, "");

						// Perform flipped captures
						if (this->parameters.capture.flip.get()) {
							Utils::ScopedProcess scopedProcessFlip("Capture flipped heliostats", false);

							// Flip all the active ones
							vector<shared_ptr<Heliostats2::Heliostat>> flippedHeliostats;
							for (auto heliostat : activeHeliostats) {
								try {
									heliostat->flip();
									flippedHeliostats.push_back(heliostat);
								}
								catch (...) {
									// this heliostat can't be flipped
								}
							}

							// Re-solve and perform capture
							if (!flippedHeliostats.empty()) {
								this->navigateToSeeBoardAndCapture(flippedHeliostats
									, heliostats
									, extrinsicsFromBoardInWorld
									, navigateSolverSettings
									, boardInWorld
									, camera
									, cameraView
									, dispatcher
									, "flip");
							}
						}
					}
					catch (const ofxRulr::Exception& exception) {
						// Turn everything away if we got an exception
						if (this->parameters.faceAway.atEnd.get()) {
							heliostats->faceAllAway();
							heliostats->update(); // Mark as stale
							heliostats->pushStale(false, false);
						}
						throw exception;
					}

					// Turn everything away at end
					if (this->parameters.faceAway.atEnd.get()) {
						heliostats->faceAllAway();
						heliostats->update(); // Mark as stale
						heliostats->pushStale(false, false);
					}
				}


				//----------
				void BoardInMirror2::navigateToSeeBoardAndCapture(vector<shared_ptr<Heliostats2::Heliostat>> activeHeliostats
					, shared_ptr<Heliostats2> heliostats
					, shared_ptr<Procedure::Calibrate::ExtrinsicsFromBoardInWorld> extrinsicsFromBoardInWorld
					, const ofxCeres::SolverSettings& navigateSolverSettings
					, shared_ptr<Item::BoardInWorld> boardInWorld
					, shared_ptr<Item::Camera> camera
					, const ofxRay::Camera& cameraView
					, shared_ptr<Dispatcher> dispatcher
					, const string& comments)
				{
					// Point the active heliostats towards the board
					if(this->parameters.capture.aimToSeenBoardPoints.get()) {
						Utils::ScopedProcess scopedProcess("Point active heliostats towards board", false);

						// Get the center of the board in world space (or only 
						glm::vec3 boardCenter(0.0f, 0.0f, 0.0f);
						{
							// We can select to only look to points that were seen directly in the camera
							// This is to avoid deviations on the estimated world positions of the world points
							// (e.g. small rotations = big deviations for other points)
							auto onlyIncludePointsInCameraView = this->parameters.capture.aimToSeenBoardPoints.get();
							auto cameraImageBounds = ofRectangle(0, 0, camera->getWidth(), camera->getHeight());

							auto worldPoints = boardInWorld->getAllWorldPoints();
							size_t count = 0;
							for (const auto& worldPoint : worldPoints) {
								if (onlyIncludePointsInCameraView) {
									// check inside camera view
									auto imagePoint = cameraView.getScreenCoordinateOfWorldPosition(worldPoint);
									if (!cameraImageBounds.inside(imagePoint)) {
										continue;
									}
								}

								boardCenter += worldPoint;
								count++;
							}
							boardCenter /= (float)count;
						}

						for (auto heliostat : activeHeliostats) {
							heliostat->navigateToReflectPointToPoint(camera->getPosition()
								, boardCenter
								, navigateSolverSettings
								, false);
						}

						// Mark stale
						heliostats->update();
						heliostats->pushStale(true, true);

						{
							Utils::ScopedProcess scopedProcessWait("Wait for heliostats to settle", false);
							ofSleepMillis(this->parameters.servoControl.waitTime * 1000.0f);
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

					// Take a fresh photo (now the photo contains the reflected boards and the original board)
					cv::Mat imageWithReflections;
					float meanBrightness;
					{
						Utils::ScopedProcess scopedProcess("Take a photo of board reflections", false);

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

						meanBrightness = cv::mean(imageWithReflections)[0];
					}

					// Mask out the direct view of the board
					cv::Mat imageWithReflectionsWithoutRealBoard = imageWithReflections.clone();
					cv::Mat imageWithRealBoardOnly = imageWithReflections.clone();
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

						// Set outside mask to mean color
						imageWithReflectionsWithoutRealBoard.setTo(cv::Scalar(meanBrightness), alphaMask);

						// Mask including board only
						{
							cv::bitwise_not(alphaMask, alphaMask);
							auto meanValue = cv::mean(imageWithRealBoardOnly, alphaMask);
							imageWithRealBoardOnly.setTo(meanValue, alphaMask);
						}
					}

					// Update real board tracking (e.g. if was blown by wind)
					if(this->parameters.findBoard.updateBoardInFinalImage.get()) {
						extrinsicsFromBoardInWorld->track(Procedure::Calibrate::ExtrinsicsFromBoardInWorld::UpdateTarget::Board
							, imageWithRealBoardOnly);
					}



					// Find boards in each heliostat mirror face
					{
						Utils::ScopedProcess scopedProcess("Find reflected boards in mirrors", false, activeHeliostats.size());

						for (auto heliostat : activeHeliostats) {
							try {
								Utils::ScopedProcess heliostatScopedProcess("Heliostat : " + heliostat->getName());
								this->processReflectionInHeliostat(heliostats
									, heliostat
									, cameraView
									, imageWithReflectionsWithoutRealBoard
									, boardInWorld
									, servoValues
									, comments
									, meanBrightness);
								heliostatScopedProcess.end();
							}
							RULR_CATCH_ALL_TO_ERROR;
						}
					}
				}

				//----------
				void BoardInMirror2::processReflectionInHeliostat(shared_ptr<Heliostats2> heliostats
					, shared_ptr<Heliostats2::Heliostat> heliostat
					, const ofxRay::Camera& cameraView
					, const cv::Mat& imageWithReflectionsWithoutRealBoard
					, shared_ptr<Item::BoardInWorld> boardInWorld
					, const map<Dispatcher::ServoID, Dispatcher::RegisterValue>& servoValues
					, const string& comments
					, float meanBrightness)
				{
					auto dispatcher = heliostats->getInput<Dispatcher>();

					// Get the mask for this mirror
					auto mask = heliostats->drawMirrorFaceMask(heliostat, cameraView, this->parameters.capture.mirrorScale.get());

					// Get the bounding box of the mask
					cv::Rect boundingBox;
					{
						cv::Mat activePixels;
						cv::findNonZero(mask, activePixels);
						boundingBox = cv::boundingRect(activePixels);
					}

					// Mask the image
					cv::Mat maskedImage = imageWithReflectionsWithoutRealBoard.clone();
					if(this->parameters.capture.applyMask.get()) {
						cv::Mat invertedMask;
						cv::bitwise_not(mask, invertedMask);
						maskedImage.setTo(cv::Scalar(meanBrightness), invertedMask);
					}

					//// Calculate if looking at front or back of board
					//bool reflectionShowsFront;
					//{
					//	const auto boardPosition = boardInWorld->getPosition();
					//	auto cameraViewMatrix = cameraView.getViewMatrix();
					//	auto boardInCamera = Utils::applyTransform(cameraViewMatrix, boardPosition);
					//	auto heliostatInCamera = Utils::applyTransform(cameraViewMatrix, heliostat->parameters.hamParameters.position.get());

					//	reflectionShowsFront = abs(boardInCamera.z) > abs(heliostatInCamera.z); // we don't need abs, we're just being lazy here to code for the correct sign
					//}

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
							, cv::NORM_MINMAX);

						// Find the board in the cropped image
						{
							bool foundBoard = false;
							
							if (this->parameters.findBoard.reflectionSeesFront.get()) {
								cv::Mat mirrored;
								cv::flip(croppedRegion
									, mirrored
									, 1);
								foundBoard = boardInWorld->findBoard(mirrored
									, imagePointsCropped
									, objectPoints
									, capture->worldPoints
									, this->parameters.findBoard.modeForReflections.get());
								

								if (!foundBoard && this->parameters.findBoard.useAssistantIfFail.get()) {
									foundBoard = boardInWorld->findBoard(mirrored
										, imagePointsCropped
										, objectPoints
										, capture->worldPoints
										, FindBoardMode::Assistant);
								}

								for (auto& imagePoint : imagePointsCropped) {
									imagePoint.x = croppedRegion.cols - imagePoint.x - 1;
								}
							}
							else {
								foundBoard = boardInWorld->findBoard(croppedRegion
									, imagePointsCropped
									, objectPoints
									, capture->worldPoints
									, this->parameters.findBoard.modeForReflections.get());

								if (!foundBoard && this->parameters.findBoard.useAssistantIfFail.get()) {
									foundBoard = boardInWorld->findBoard(croppedRegion
										, imagePointsCropped
										, objectPoints
										, capture->worldPoints
										, FindBoardMode::Assistant);
								}
							}

							
							if (!foundBoard) {
								throw(Exception("Failed to find board in reflection"));
							}
						}

						if (imagePointsCropped.size() == 0) {
							throw(Exception("Failed to find board in reflection"));
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

					capture->heliostatName = heliostat->getName();
					capture->cameraPosition = cameraView.getPosition();
					capture->comments = comments;
					capture->color = heliostat->color;

					// Store the camera view
					{
						capture->camera = cameraView;
						capture->camera.color = capture->color;
						capture->camera.setFarClip(0.2f);
					}

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
						auto mirrorPlane = Solvers::HeliostatActionModel::getMirrorPlane<float>({
							heliostat->parameters.servo1.angle.get()
							, heliostat->parameters.servo2.angle.get()
							}, heliostat->getHeliostatActionModelParameters());
						ofxRay::Plane plane(mirrorPlane.center, mirrorPlane.normal);
						plane.setInfinite(true);

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

				//--------
				void BoardInMirror2::calibrate() {
					this->throwIfMissingAConnection<Heliostats2>();
					auto heliostatsNode = this->getInput<Heliostats2>();
					auto heliostats = heliostatsNode->getHeliostats();

					Utils::ScopedProcess calibrateScopedProcess("Calibrate", false, heliostats.size());
					for (auto heliostat : heliostats) {
						try {
							Utils::ScopedProcess heliostatScopedProcess(heliostat->getName(), true, heliostats.size());

							// Gather data
							vector<ofxRay::Ray> cameraRays;
							vector<glm::vec3> worldPoints;
							vector<int> axis1ServoPosition;
							vector<int> axis2ServoPosition;

							vector<float> axis1AngleOffset;
							vector<float> axis2AngleOffset;

							auto allCaptures = this->captures.getSelection();
							vector<shared_ptr<Capture>> capturesForThisHeliostat;

							for (const auto& capture : allCaptures) {
								if (capture->heliostatName == heliostat->getName()) {
									capturesForThisHeliostat.push_back(capture);

									//Check sizes
									auto size = capture->cameraRays.size();
									if (size != capture->worldPoints.size()) {
										throw(ofxRulr::Exception("Gathering data size mismatch"));
									}

									cameraRays.insert(cameraRays.end(), capture->cameraRays.begin(), capture->cameraRays.end());

									const auto& worldPointsToAdd = ofxCv::toOf(capture->worldPoints);
									worldPoints.insert(worldPoints.end(), worldPointsToAdd.begin(), worldPointsToAdd.end());

									for (size_t i = 0; i < size; i++) {
										axis1ServoPosition.push_back(capture->axis1ServoPosition);
										axis2ServoPosition.push_back(capture->axis2ServoPosition);
										axis1AngleOffset.push_back(heliostat->parameters.servo1.angleOffset.get());
										axis2AngleOffset.push_back(heliostat->parameters.servo2.angleOffset.get());
									}
								}
							}

							// Check that we got some data
							if (cameraRays.empty()) {
								throw(ofxRulr::Exception("No data available"));
							}

							// Check that we have enough data
							if (cameraRays.size() < this->parameters.calibrate.minimumDataPoints.get()) {
								throw(ofxRulr::Exception("[" + ofToString(cameraRays.size()) + "] data points found. Minimum [" + ofToString(this->parameters.calibrate.minimumDataPoints.get()) + "] data points required."));
							}

							// Normalise rays
							{
								for (auto& cameraRay : cameraRays) {
									cameraRay.t = glm::normalize(cameraRay.t);
								}
							}

							// Collate options
							Solvers::HeliostatActionModel::Calibrator::Options options;
							{
								options.fixPosition = this->parameters.calibrate.fixPosition.get();
								options.fixRotationY = this->parameters.calibrate.fixRotationY.get();
								options.fixMirrorOffset = this->parameters.calibrate.fixMirrorOffset.get();
								options.fixPolynomial = this->parameters.calibrate.fixPolynomial.get();
								options.fixRotationAxis = this->parameters.calibrate.fixRotationAxis.get();
								options.mirrorDiameter = heliostat->parameters.diameter.get();
							}

							// Run solve
							{
								auto solverSettings = this->getCalibrateSolverSettings();
								{
									solverSettings.options.max_num_iterations = this->parameters.calibrate.maxIterations.get();
									solverSettings.options.function_tolerance = this->parameters.calibrate.functionTolerance.get();
									solverSettings.options.parameter_tolerance = this->parameters.calibrate.parameterTolerance.get();
								}

								auto result = Solvers::HeliostatActionModel::Calibrator::solveCalibration(cameraRays
									, worldPoints
									, axis1ServoPosition
									, axis2ServoPosition
									, axis1AngleOffset
									, axis2AngleOffset
									, heliostat->getHeliostatActionModelParameters()
									, options
									, solverSettings);

								if (result.isConverged()) {
									// Update the 
									heliostat->setHeliostatActionModelParameters(result.solution);

									// Update the residuals
									this->calculateResiduals();

									// Calculate the reflections
									for (auto& capture : capturesForThisHeliostat) {
										// Clear prior reflections
										capture->reflectedRays.clear();

										// Create the board plane
										Solvers::HeliostatActionModel::AxisAngles<float> axisAngles{
											Solvers::HeliostatActionModel::positionToAngle<float>(capture->axis1ServoPosition
												, heliostat->parameters.hamParameters.axis1.polynomial.get()
												, heliostat->parameters.servo1.angleOffset.get())
											, Solvers::HeliostatActionModel::positionToAngle<float>(capture->axis2ServoPosition
												, heliostat->parameters.hamParameters.axis2.polynomial.get()
												, heliostat->parameters.servo2.angleOffset.get())
										};

										// Get mirror plane
										auto mirrorPlane_ = Solvers::HeliostatActionModel::getMirrorPlane(axisAngles
											, heliostat->getHeliostatActionModelParameters());
										ofxRay::Plane mirrorPlane(mirrorPlane_.center, mirrorPlane_.normal);
										mirrorPlane.setInfinite(true);

										// Calculate the reflections
										for (const auto& cameraRay : capture->cameraRays) {
											glm::vec3 intersection;
											auto transmission = glm::normalize(cameraRay.t);
											if (mirrorPlane.intersect(cameraRay, intersection) || true) {
												auto transmissionReflected = mirrorPlane.reflect(intersection + transmission) - intersection;

												// Start with a copy of the cameraRay and edit
												capture->reflectedRays.push_back(cameraRay);
												auto& ray = capture->reflectedRays.back();
												ray.s = intersection;
												ray.t = transmissionReflected;
											}
										}

									}
								}
							}

							heliostatScopedProcess.end();
						}
						RULR_CATCH_ALL_TO_ERROR;
					}

					{
						Utils::ScopedProcess scopedProcessPullLimits("Pull all limits (using new calibration)");
						heliostatsNode->pullAllLimits();
						scopedProcessPullLimits.end();
					}
				}

				//--------
				ofxCvGui::PanelPtr BoardInMirror2::getPanel() {
					return this->panel;
				}

				//--------
				void BoardInMirror2::populateInspector(ofxCvGui::InspectArguments& inspectArgs) {
					auto inspector = inspectArgs.inspector;
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

					inspector->addButton("Calculate residuals", [this]() {
						try {
							this->calculateResiduals();
						}
						RULR_CATCH_ALL_TO_ALERT
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
					// Draw capture data
					{
						Capture::DrawOptions drawOptions;
						drawOptions.camera = this->parameters.debug.draw.cameras.get();
						drawOptions.cameraRays = this->parameters.debug.draw.cameraRays.get();
						drawOptions.worldPoints = this->parameters.debug.draw.worldPoints.get();
						drawOptions.worldPointsSize = this->parameters.debug.draw.worldPointsSize.get();
						drawOptions.labelWorldPoints = this->parameters.debug.draw.labelWorldPoints.get();
						drawOptions.reflectedRays = this->parameters.debug.draw.reflectedRays.get();
						drawOptions.mirrorFace = this->parameters.debug.draw.mirrorFace.get();
						auto captures = this->captures.getSelection();
						for (const auto& capture : captures) {
							capture->drawWorld(drawOptions);
						}
					}

					// Draw reports
					if (this->parameters.debug.draw.report.get()) {
						auto heliostatsNode = this->getInput<Heliostats2>();
						if (heliostatsNode) {
							auto heliostats = heliostatsNode->getHeliostats();
							auto captures = this->captures.getSelection();
							for (auto heliostat : heliostats) {
								vector<shared_ptr<Capture>> gatheredCaptures;
								size_t totalCount = 0;
								size_t captureCount = 0;
								size_t flipCount = 0;
								for (auto capture : captures) {
									if (capture->heliostatName == heliostat->parameters.name.get()) {
										gatheredCaptures.push_back(capture);
										totalCount += capture->imagePoints.size();
										captureCount++;
										if (capture->comments == "flip") {
											flipCount++;
										}
									}
								}
								stringstream report;
								report << "T" << totalCount << " ";
								report << "C" << captureCount << " ";
								if (flipCount > 0) {
									report << "F" << flipCount;
								}

								ofxCvGui::Utils::drawTextAnnotation(report.str()
									, heliostat->parameters.hamParameters.position.get()
									, heliostat->color);
							}
						}
					}
				}

				//--------
				void
					BoardInMirror2::filterCapturesToHeliostatSelection() 
				{
					this->throwIfMissingAConnection<Heliostats2>();
					auto heliostatsNode = this->getInput<Heliostats2>();
					auto activeHeliostats = heliostatsNode->getHeliostats();

					auto captures = this->captures.getSelection();
					for (auto capture : captures) {
						bool isForSelectedHeliostat = false;
						for (auto heliostat : activeHeliostats) {
							if (heliostat->parameters.name.get() == capture->heliostatName) {
								isForSelectedHeliostat = true;
								break;
							}
							if (!isForSelectedHeliostat) {
								capture->setSelected(false);
							}
						}
					}
				}

				//--------
				void
					BoardInMirror2::calculateResiduals()
				{
					this->throwIfMissingAConnection<Heliostats2>();
					auto heliostatsNode = this->getInput<Heliostats2>();
					auto activeHeliostats = heliostatsNode->getHeliostats();

					auto captures = this->captures.getSelection();

					for (auto& capture : captures) {
						shared_ptr<Heliostats2::Heliostat> heliostat;
						for (auto activeHeliostat : activeHeliostats) {
							if (activeHeliostat->parameters.name.get() == capture->heliostatName) {
								heliostat = activeHeliostat;
								break;
							}
						}
						if (!heliostat) {
							continue;
						}

						float totalResidual = 0.0f;
						for (size_t i = 0; i < capture->cameraRays.size(); i++) {
							auto rayResidual = Solvers::HeliostatActionModel::Calibrator::getResidual(capture->cameraRays[i]
								, ofxCv::toOf(capture->worldPoints[i])
								, capture->axis1ServoPosition
								, capture->axis2ServoPosition
								, heliostat->parameters.servo1.angleOffset.get()
								, heliostat->parameters.servo2.angleOffset.get()
								, heliostat->getHeliostatActionModelParameters()
								, heliostat->parameters.diameter.get());
							totalResidual += rayResidual;
							capture->residuals.push_back(rayResidual);
						}
						capture->meanResidual = totalResidual / (float)capture->cameraRays.size();
					}
				}

				//--------
				void
					BoardInMirror2::filterHeliostatsToInsideCamera()
				{
					this->throwIfMissingAConnection<Item::Camera>();
					this->throwIfMissingAConnection<Heliostats2>();

					auto heliostatsNode = this->getInput<Heliostats2>();
					auto cameraNode = this->getInput<Item::Camera>();

					auto heliostats = heliostatsNode->getHeliostats();
					auto cameraView = cameraNode->getViewInWorldSpace();

					for (auto heliostat : heliostats) {
						auto heliostatInCamera = cameraView.getNormalizedSCoordinateOfWorldPosition(heliostat->parameters.hamParameters.position.get());
						if (abs(heliostatInCamera.x) > 1.0f
							|| abs(heliostatInCamera.y) > 1.0f) {
							heliostat->setSelected(false);
						}
					}
				}

				//----------
				ofxCeres::SolverSettings
					BoardInMirror2::getNavigateSolverSettings() const 
				{
					auto solverSettings = ofxRulr::Solvers::HeliostatActionModel::Navigator::defaultSolverSettings();

					solverSettings.options.max_num_iterations = this->parameters.navigate.maxIterations.get();
					solverSettings.printReport = this->parameters.navigate.printReport.get();
					solverSettings.options.minimizer_progress_to_stdout = this->parameters.navigate.printReport.get();

					return solverSettings;
				}

				//----------
				ofxCeres::SolverSettings BoardInMirror2::getCalibrateSolverSettings() const {
					auto solverSettings = ofxRulr::Solvers::HeliostatActionModel::Calibrator::defaultSolverSettings();

					solverSettings.options.max_num_iterations = this->parameters.calibrate.maxIterations.get();
					solverSettings.printReport = this->parameters.calibrate.printReport.get();
					solverSettings.options.minimizer_progress_to_stdout = this->parameters.calibrate.printReport.get();

					return solverSettings;
				}
			}
		}
	}
}
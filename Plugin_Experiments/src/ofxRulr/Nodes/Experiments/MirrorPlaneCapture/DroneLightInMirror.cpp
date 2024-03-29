#include "pch_Plugin_Experiments.h"
#include "DroneLightInMirror.h"

namespace ofxRulr {
	namespace Nodes {
		namespace Experiments {
			namespace MirrorPlaneCapture {
#pragma mark Capture
				//-----------
				DroneLightInMirror::Capture::Capture()
				{
					RULR_SERIALIZE_LISTENERS;
				}

				//-----------
				void
					DroneLightInMirror::Capture::drawWorld(const DrawOptions& args)
				{
					vector<HeliostatCapture> selectedHeliostatCapture;
					if (args.selectedHeliostatsOnly) {
						for (auto heliostatCapture : this->heliostatCaptures) {
							if (args.selectedHelisotats.find(heliostatCapture.heliostatName) != args.selectedHelisotats.end()) {
								// in selection
								selectedHeliostatCapture.push_back(heliostatCapture);
							}
						}
					}

					// If selectedHeliostatsOnly and we don't have any, don't draw
					if (args.selectedHeliostatsOnly && selectedHeliostatCapture.empty()) {
						return;
					}

					ofPushStyle();
					{
						ofSetColor(this->color);

						if (args.camera) {
							this->camera.draw();
						}

						if (args.lightPositions) {
							ofDrawSphere(this->lightPosition, args.worldPointsSize);
						}

						if (args.labelWorldPoints) {
							ofxCvGui::Utils::drawTextAnnotation(this->getDateString()
								, this->camera.getPosition()
								, this->color);
						}

						for (const auto& heliostatCapture : this->heliostatCaptures) {
							if (args.selectedHelisotats.find(heliostatCapture.heliostatName) == args.selectedHelisotats.end()) {
								// not selected
								continue;
							}

							if (args.cameraRays) {
								heliostatCapture.cameraRay.draw();
							}

							if (args.cameraRaysShort) {
								// Get the ray
								auto ray = heliostatCapture.cameraRay;
								auto totalCameraRayEnd = ray.getEnd();

								// Draw the ray close to the camera
								{
									ray.t *= 0.1f / glm::length(ray.t);
									ray.draw();
								}

								// Dray the ray close to the mirror
								{
									ray.s = totalCameraRayEnd - ray.t;
									ray.draw();
								}
							}

							if (args.mirrorFace) {
								// Not implemented
							}

							if (args.reflectedRays) {
								heliostatCapture.reflectedRayEstimated.draw();
							}
						}

						if (args.mirrorFace) {
							for (const auto& heliostatCapture : heliostatCaptures) {
								ofDrawArrow(heliostatCapture.mirrorCenterEstimated
									, heliostatCapture.mirrorNormalEstimated * 0.1 + heliostatCapture.mirrorCenterEstimated
									, 0.01f);
							}
						}
					}
					ofPopStyle();
				}

				//-----------
				string
					DroneLightInMirror::Capture::getDisplayString() const
				{
					stringstream ss;
					for (const auto& heliostatCapture : this->heliostatCaptures) {
						ss << heliostatCapture.heliostatName << "(" << heliostatCapture.residual << "), ";
					}
					return ss.str();
				}

				//-----------
				void
					DroneLightInMirror::Capture::serialize(nlohmann::json& json)
				{
					Utils::serialize(json, "comments", this->comments);
					Utils::serialize(json, "cameraPosition", this->cameraPosition);
					Utils::serialize(json, "lightPosition", this->lightPosition);
					Utils::serialize(json, "camera", this->camera);

					auto& jsonHeliostatCaptures = json["heliostatCaptures"];
					{
						for (auto& heliostatCapture : this->heliostatCaptures) {
							nlohmann::json jsonHeliostatCapture;
							Utils::serialize(jsonHeliostatCapture, "heliostatName", heliostatCapture.heliostatName);
							Utils::serialize(jsonHeliostatCapture, "imagePoint", heliostatCapture.imagePoint);
							Utils::serialize(jsonHeliostatCapture, "cameraRay", heliostatCapture.cameraRay);
							Utils::serialize(jsonHeliostatCapture, "mirrorCenterEstimated", heliostatCapture.mirrorCenterEstimated);
							Utils::serialize(jsonHeliostatCapture, "mirrorNormalEstimated", heliostatCapture.mirrorNormalEstimated);
							Utils::serialize(jsonHeliostatCapture, "reflectedRayEstimated", heliostatCapture.reflectedRayEstimated);
							Utils::serialize(jsonHeliostatCapture, "axis1ServoPosition", heliostatCapture.axis1ServoPosition);
							Utils::serialize(jsonHeliostatCapture, "axis2ServoPosition", heliostatCapture.axis2ServoPosition);
							Utils::serialize(jsonHeliostatCapture, "axis1AngleOffest", heliostatCapture.axis1AngleOffest);
							Utils::serialize(jsonHeliostatCapture, "axis2AngleOffest", heliostatCapture.axis2AngleOffest);
							Utils::serialize(jsonHeliostatCapture, "residual", heliostatCapture.residual);
							jsonHeliostatCaptures.push_back(jsonHeliostatCapture);
						}
					}
				}

				//-----------
				void
					DroneLightInMirror::Capture::deserialize(const nlohmann::json& json)
				{
					Utils::deserialize(json, "comments", this->comments);
					Utils::deserialize(json, "cameraPosition", this->cameraPosition);
					Utils::deserialize(json, "lightPosition", this->lightPosition);
					Utils::deserialize(json, "camera", this->camera);

					this->heliostatCaptures.clear();
					if (json.contains("heliostatCaptures")) {
						const auto& jsonHeliostatCaptures = json["heliostatCaptures"];

						for (auto& jsonHeliostatCapture : jsonHeliostatCaptures) {
							HeliostatCapture heliostatCapture;
							Utils::deserialize(jsonHeliostatCapture, "heliostatName", heliostatCapture.heliostatName);
							Utils::deserialize(jsonHeliostatCapture, "imagePoint", heliostatCapture.imagePoint);
							Utils::deserialize(jsonHeliostatCapture, "cameraRay", heliostatCapture.cameraRay);
							Utils::deserialize(jsonHeliostatCapture, "mirrorCenterEstimated", heliostatCapture.mirrorCenterEstimated);
							Utils::deserialize(jsonHeliostatCapture, "mirrorNormalEstimated", heliostatCapture.mirrorNormalEstimated);
							Utils::deserialize(jsonHeliostatCapture, "reflectedRayEstimated", heliostatCapture.reflectedRayEstimated);
							Utils::deserialize(jsonHeliostatCapture, "axis1ServoPosition", heliostatCapture.axis1ServoPosition);
							Utils::deserialize(jsonHeliostatCapture, "axis2ServoPosition", heliostatCapture.axis2ServoPosition);
							Utils::deserialize(jsonHeliostatCapture, "axis1AngleOffest", heliostatCapture.axis1AngleOffest);
							Utils::deserialize(jsonHeliostatCapture, "axis2AngleOffest", heliostatCapture.axis2AngleOffest);
							Utils::deserialize(jsonHeliostatCapture, "residual", heliostatCapture.residual);
							this->heliostatCaptures.push_back(heliostatCapture);
						}
					}
				}

#pragma mark DroneLightInMirror
				//-----------
				DroneLightInMirror::DroneLightInMirror()
				{
					RULR_NODE_INIT_LISTENER;
				}

				//-----------
				string
					DroneLightInMirror::getTypeName() const
				{
					return "Halo::DroneLightInMirror";
				}

				//-----------
				void
					DroneLightInMirror::init()
				{
					RULR_NODE_UPDATE_LISTENER;
					RULR_NODE_INSPECTOR_LISTENER;
					RULR_NODE_DRAW_WORLD_LISTENER;
					RULR_NODE_SERIALIZATION_LISTENERS;

					this->manageParameters(this->parameters);

					this->addInput<Item::Camera>();
					this->addInput<Dispatcher>();
					this->addInput<Heliostats2>();
					this->addInput<FindLightInMirror>();
					this->addInput<MarkerMap::NavigateCamera>();

					this->panel = ofxCvGui::Panels::makeWidgets();
					this->captures.populateWidgets(this->panel);
				}

				//-----------
				void
					DroneLightInMirror::update()
				{
					// Navigate to see the light on any fresh frame from camera
					if (this->parameters.navigate.onFreshFrame && !this->lastFrameWasCapture) {
						try {
							auto camera = this->getInput<Item::Camera>();
							if (camera) {
								if (camera->getGrabber()->isFrameNew()) {
									this->navigateToSeeLight();
								}
							}
						}
						RULR_CATCH_ALL_TO_ERROR;
					}

					this->lastFrameWasCapture = false;
				}

				//-----------
				void
					DroneLightInMirror::populateInspector(ofxCvGui::InspectArguments& args)
				{
					auto inspector = args.inspector;

					inspector->addButton("Navigate to light", [this]() {
						try {
							this->navigateToSeeLight();
						}
						RULR_CATCH_ALL_TO_ALERT;
						}, 'n');

					inspector->addButton("Capture", [this]() {
						try {
							this->capture();
						}
						RULR_CATCH_ALL_TO_ALERT;
						}, ' ');

					inspector->addButton("Calibrate", [this]() {
						try {
							this->calibrate();
						}
						RULR_CATCH_ALL_TO_ALERT;
						}, OF_KEY_RETURN)->setHeight(100.0f);

					inspector->addButton("Calculate residuals", [this]() {
						try {
							this->calculateResiduals();
						}
						RULR_CATCH_ALL_TO_ALERT;
						}, 'r');

					inspector->addLiveValue<float>("Total residual", [this]() {
						auto captures = this->captures.getSelection();
						float totalResidual = 0.0f;
						for (auto capture : captures) {
							totalResidual += capture->meanResidual;
						}
						return totalResidual;
						});
				}

				//-----------
				ofxCvGui::PanelPtr
					DroneLightInMirror::getPanel()
				{
					return this->panel;
				}

				//-----------
				void
					DroneLightInMirror::drawWorldStage()
				{
					// Draw the current light
					if (this->parameters.debug.draw.light) {
						auto cameraNode = this->getInput<Item::Camera>();
						if (cameraNode) {
							auto cameraTransform = cameraNode->getTransform();
							auto lightPosition = this->getLightPosition(cameraTransform);

							ofPushMatrix();
							{
								ofTranslate(lightPosition);
								ofDrawLine(-0.02, 0, 0.02, 0);
								ofDrawLine(0, -0.02, 0, 0.02);
								ofDrawLine(0, 0, -0.02, 0, 0, 0.02);
							}
							ofPopMatrix();
						}
					}

					// Draw captures
					{
						Capture::DrawOptions drawOptions;
						{
							// selected heliostats
							{
								auto heliostatsNode = this->getInput<Heliostats2>();
								if (heliostatsNode) {
									auto heliostats = heliostatsNode->getHeliostats();
									for (auto heliostat : heliostats) {
										drawOptions.selectedHelisotats.insert(heliostat->parameters.name.get());
									}
								}
							}
							
							drawOptions.camera = this->parameters.debug.draw.cameras.get();
							drawOptions.cameraRays = this->parameters.debug.draw.cameraRays.get();
							drawOptions.cameraRaysShort = this->parameters.debug.draw.cameraRaysShort.get();
							drawOptions.lightPositions = this->parameters.debug.draw.lightPositions.get();
							drawOptions.worldPointsSize = this->parameters.debug.draw.worldPointsSize.get();
							drawOptions.labelWorldPoints = this->parameters.debug.draw.labelWorldPoints.get();
							drawOptions.reflectedRays = this->parameters.debug.draw.reflectedRays.get();
							drawOptions.mirrorFace = this->parameters.debug.draw.mirrorFace.get();
						}
						auto captures = this->captures.getSelection();
						for (auto capture : captures) {
							capture->drawWorld(drawOptions);
						}
					}

					// Draw report
					if (this->parameters.debug.draw.report) {
						// Build the report
						map<string, int> countPerHeliostat;
						map<string, shared_ptr<Heliostats2::Heliostat>> heliostatsPerName;
						{
							// Get heliostats
							auto heliostatsNode = this->getInput<Heliostats2>();
							if (heliostatsNode) {
								auto heliostats = heliostatsNode->getHeliostats();
								for (auto heliostat : heliostats) {
									const auto& name = heliostat->parameters.name.get();
									heliostatsPerName.emplace(name, heliostat);
								}
							}

							// Count captures
							auto captures = this->captures.getSelection();
							for (auto capture : captures) {
								for (auto heliostatCapture : capture->heliostatCaptures) {
									const auto& heliostatName = heliostatCapture.heliostatName;
									// See if already exists
									if (countPerHeliostat.find(heliostatName) == countPerHeliostat.end()) {
										// doesn't exit
										countPerHeliostat[heliostatName] = 0;
									}
									else {
										// does exist
										countPerHeliostat[heliostatName]++;
									}
								}
							}
						}

						// Draw the report
						for (auto it : countPerHeliostat) {
							auto name = it.first;
							if (heliostatsPerName.find(name) == heliostatsPerName.end()) {
								// this heliostat is not currently in selection
								continue;
							}
							auto heliostat = heliostatsPerName[name];
							auto position = heliostat->parameters.hamParameters.position.get();
							auto count = it.second;
							// make color
							ofColor color;
							if (count < 8) {
								color = ofColor(255, 50, 50);
							}
							else if (count < 12) {
								color = ofColor(200, 100, 200);
							}
							else if (count < 16) {
								color = ofColor(150, 150, 255);
							}
							else if (count < 20) {
								color = ofColor(100, 255, 100);
							}
							else {
								color = ofColor(255);
							}
							ofxCvGui::Utils::drawTextAnnotation("H" + it.first + " : n=" + ofToString(count), position, color);
						}
					}
				}

				//-----------
				void
					DroneLightInMirror::serialize(nlohmann::json& json)
				{
					this->captures.serialize(json["captures"]);
				}

				//-----------
				void
					DroneLightInMirror::deserialize(const nlohmann::json& json)
				{
					if (json.contains("captures")) {
						this->captures.deserialize(json["captures"]);
					}
				}

				//-----------
				void
					DroneLightInMirror::navigateToSeeLight()
				{
					Utils::ScopedProcess scopedProcess("Navigate to see light");

					this->throwIfMissingAConnection<Item::Camera>();
					this->throwIfMissingAConnection<Heliostats2>();

					auto cameraNode = this->getInput<Item::Camera>();

					if (this->parameters.cameraNavigation.whenSeeLight.get()) {
						Utils::ScopedProcess scopedProcessNavigateCamera("Navigate camera");

						this->throwIfMissingAConnection<MarkerMap::NavigateCamera>();
						auto navigateCamera = this->getInput<MarkerMap::NavigateCamera>();

						auto frame = cameraNode->getFrame();
						if (!frame) {
							throw(ofxRulr::Exception("No camera frame available"));
						}
						auto image = ofxCv::toCv(frame->getPixels());
						navigateCamera->track(image, this->parameters.cameraNavigation.trustPriorPose.get());

						scopedProcessNavigateCamera.end();
					}

					// Get inputs
					auto heliostatsNode = this->getInput<Heliostats2>();
					auto cameraView = cameraNode->getViewInWorldSpace();

					// Gather heliostats
					auto heliostats = heliostatsNode->getHeliostats();

					// Filter to heliostats in the camera view
					auto heliostatsInView = DroneLightInMirror::getHeliostatsInView(heliostats, cameraView);

					// Call navigate on heliostats
					{
						Utils::ScopedProcess scopedProcessNavigateHeliostats("Navigate heliostats");
						this->navigateToSeeLight(heliostats
							, heliostatsNode
							, this->getNavigateSolverSettings());
						scopedProcessNavigateHeliostats.end();
					}

					scopedProcess.end();
				}

				//-----------
				void
					DroneLightInMirror::navigateToSeeLight(vector<shared_ptr<Heliostats2::Heliostat>> heliostats
						, shared_ptr<Heliostats2> heliostatsNode
						, const ofxCeres::SolverSettings& navigateSolverSettings)
				{
					auto cameraNode = this->getInput<Item::Camera>();

					auto cameraView = cameraNode->getViewInWorldSpace();
					auto cameraPosition = cameraView.getPosition();

					// Calculate the light position
					auto lightPosition = this->getLightPosition(cameraNode->getTransform());

					for (auto heliostat : heliostats) {
						heliostat->navigateToReflectPointToPoint(cameraPosition
							, lightPosition
							, this->getNavigateSolverSettings()
							, false);
					}
				}

				//-----------
				void
					DroneLightInMirror::capture()
				{
					Utils::ScopedProcess scopedProcess("Capture");

					this->lastFrameWasCapture = true;

					this->throwIfMissingAnyConnection();

					auto dispatcher = this->getInput<Dispatcher>();
					auto heliostatsNode = this->getInput<Heliostats2>();
					auto cameraNode = this->getInput<Item::Camera>();
					auto findLightInMirror = this->getInput<FindLightInMirror>();
					auto navigateCamera = this->getInput<MarkerMap::NavigateCamera>();


					// Get camera frame image as greyscale
					shared_ptr<ofxMachineVision::Frame> frame;
					cv::Mat image;
					{
						Utils::ScopedProcess scopedProcessGetCameraFrame("Get camera frame");

						// Get grabber and check if it's open
						auto grabber = cameraNode->getGrabber();
						if (!grabber->getIsDeviceOpen()) {
							throw(ofxRulr::Exception("Camera is not open"));
						}

						// If we need to wait for a new frame or use existing one
						frame = this->parameters.capture.waitForNewFrame.get()
							? grabber->getFreshFrame()
							: grabber->getFrame();

						auto& pixels = frame->getPixels();
						if (pixels.getNumChannels() > 1) {
							// RGB image
							cv::cvtColor(ofxCv::toCv(pixels), image, cv::COLOR_RGB2GRAY);
						}
						else {
							// Greyscale image (not we don't necessarilly copy data here, so make sure to keep the frame in memory)
							image = ofxCv::toCv(pixels);
						}

						scopedProcessGetCameraFrame.end();
					}

					// Gather heliostats in view
					auto priorCameraView = cameraNode->getViewInWorldSpace();
					auto heliostatsInView = DroneLightInMirror::getHeliostatsInView(heliostatsNode->getHeliostats()
						, priorCameraView);
					auto count = heliostatsInView.size();

					if (heliostatsInView.empty()) {
						throw(ofxRulr::Exception("No heliostats in view"));
					}

					// Get servo values for our heliostats
					map<Dispatcher::ServoID, Dispatcher::RegisterValue> servoValues;
					{
						vector<Dispatcher::ServoID> servoIDs;

						// gather servo IDs for our heliostats
						for (auto heliostat : heliostatsInView) {
							servoIDs.push_back(heliostat->parameters.servo1.ID.get());
							servoIDs.push_back(heliostat->parameters.servo2.ID.get());
						}

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

					// Navigate the camera to the correct position using marker map
					if (this->parameters.cameraNavigation.whenCapture) {
						Utils::ScopedProcess scopedProcessNavigateCamera("Navigate camera");
						navigateCamera->track(image, this->parameters.cameraNavigation.trustPriorPose.get());
						scopedProcessNavigateCamera.end();
					}

					// Get the camera view after navigating
					auto cameraView = cameraNode->getViewInWorldSpace();

					// Find the light in the image
					vector<FindLightInMirror::Result> findLightResults;
					{
						Utils::ScopedProcess scopedProcessFindLights("Find lights");

						FindLightInMirror::FindSettings findSettings;
						{
							findSettings.buildPreview = this->parameters.capture.debugFindLight.get();
						}
						findLightResults = findLightInMirror->findLights(image
							, heliostatsInView
							, heliostatsNode
							, findSettings);

						scopedProcessFindLights.end();
					}

					// Check if no lights were seen
					{
						bool seenAny = false;
						for (const auto& result : findLightResults) {
							if (result.success) {
								seenAny = true;
								break;
							}
						}

						if (!seenAny) {
							throw(ofxRulr::Exception("Didn't see light in any of the mirrors"));
						}
					}

					// Build up the capture
					{
						Utils::ScopedProcess scopedProcessBuildCapture("Build capture");

						auto capture = make_shared<Capture>();

						capture->cameraPosition = cameraView.getPosition();
						capture->lightPosition = this->getLightPosition(cameraNode->getTransform());

						{
							capture->camera = cameraView;
							capture->camera.setFarClip(0.2f);
							capture->camera.color = capture->color;
						}

						// Function to find axis servo values
						auto findServoValue = [&](const int& servoID) {
							auto findServoValue = servoValues.find(servoID);
							if (findServoValue == servoValues.end()) {
								throw(ofxRulr::Exception("Failed to get a Present Position for Servo [" + ofToString(servoID) + "]"));
							}
							return findServoValue->second;
						};

						// Gather heliostat captures
						for (int i = 0; i < count; i++) {
							auto& findLightResult = findLightResults[i];

							// Ignore if light not seen in this heliostat
							if (!findLightResult.success) {
								continue;
							}

							// Store the heliostat captures
							Capture::HeliostatCapture heliostatCapture;
							{
								auto& heliostat = heliostatsInView[i];

								heliostatCapture.axis1ServoPosition = findServoValue(heliostat->parameters.servo1.ID.get());
								heliostatCapture.axis2ServoPosition = findServoValue(heliostat->parameters.servo2.ID.get());
								heliostatCapture.axis1AngleOffest = heliostat->parameters.servo1.angleOffset.get();
								heliostatCapture.axis2AngleOffest = heliostat->parameters.servo2.angleOffset.get();

								heliostatCapture.heliostatName = heliostat->parameters.name.get();
								heliostatCapture.imagePoint = findLightResult.imagePoint;

								auto cameraRay = cameraView.castPixel(findLightResult.imagePoint, true);
								cameraRay.color = capture->color;

								// Create the approx results based on current esimated
								auto mirrorPlane = Solvers::HeliostatActionModel::getMirrorPlane<float>({
									heliostat->parameters.servo1.angle.get()
									, heliostat->parameters.servo2.angle.get()
									}, heliostat->getHeliostatActionModelParameters());
								ofxRay::Plane plane(mirrorPlane.center, mirrorPlane.normal);

								// Crop the cameraRay to hit the current mirror estimation
								{
									glm::vec3 rayEnd;
									if (plane.intersect(heliostatCapture.cameraRay, rayEnd)) {
										cameraRay.setEnd(rayEnd);
									}
									cameraRay.infinite = false;
								}

								heliostatCapture.cameraRay = cameraRay;

								heliostatCapture.mirrorCenterEstimated = mirrorPlane.center;
								heliostatCapture.mirrorNormalEstimated = mirrorPlane.normal;
							}
							capture->heliostatCaptures.push_back(heliostatCapture);
						}

						this->captures.add(capture);
						scopedProcessBuildCapture.end();
					}

					scopedProcess.end();
				}

				//-----------
				void
					DroneLightInMirror::calibrate()
				{
					this->throwIfMissingAConnection<Heliostats2>();
					auto heliostatsNode = this->getInput<Heliostats2>();
					auto heliostats = heliostatsNode->getHeliostats();

					Utils::ScopedProcess calibrateScopedProcess("Calibrate", false, heliostats.size());

					const auto allCaptures = this->captures.getSelection();

					for (auto heliostat : heliostats) {
						try {
							Utils::ScopedProcess heliostatScopedProcess(heliostat->getName(), true);

							// Gather data
							vector<ofxRay::Ray> cameraRays;
							vector<glm::vec3> worldPoints;
							vector<int> axis1ServoPosition;
							vector<int> axis2ServoPosition;

							vector<float> axis1AngleOffset;
							vector<float> axis2AngleOffset;

							vector<shared_ptr<Capture>> capturesForThisHeliostat;

							for (const auto& capture : allCaptures) {
								for (const auto& heliostatCapture : capture->heliostatCaptures) {
									if (heliostatCapture.heliostatName == heliostat->getName()) {
										capturesForThisHeliostat.push_back(capture);

										cameraRays.push_back(heliostatCapture.cameraRay);
										worldPoints.push_back(capture->lightPosition);

										axis1ServoPosition.push_back(heliostatCapture.axis1ServoPosition);
										axis2ServoPosition.push_back(heliostatCapture.axis2ServoPosition);
										axis1AngleOffset.push_back(heliostatCapture.axis1AngleOffest);
										axis2AngleOffset.push_back(heliostatCapture.axis2AngleOffest);
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
								}
							}

							{
								Utils::ScopedProcess scopedProcessCalculateResiduals("Calculate residuals");
								// Update the residuals
								this->calculateResiduals();
								scopedProcessCalculateResiduals.end();
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

				//-----------
				vector<shared_ptr<Heliostats2::Heliostat>>
					DroneLightInMirror::getHeliostatsInView(vector<shared_ptr<Heliostats2::Heliostat>> heliostats
						, const ofxRay::Camera& cameraView)
				{
					vector<shared_ptr<Heliostats2::Heliostat>> heliostatsInView;
					{
						for (auto heliostat : heliostats) {
							auto position = heliostat->parameters.hamParameters.position.get();
							auto positionInViewSNorm = cameraView.getNormalizedSCoordinateOfWorldPosition(position);
							if (abs(positionInViewSNorm.x) <= 1.0f && abs(positionInViewSNorm.y) <= 1.0f) {
								heliostatsInView.push_back(heliostat);
							}
						}
					}
					return heliostatsInView;
				}

				//-----------
				glm::vec3
					DroneLightInMirror::getLightPosition(const glm::mat4& cameraTransform)
				{
					const auto& cameraPosition = ofxCeres::VectorMath::applyTransform(cameraTransform, { 0, 0, 0 });

					auto findAxis = [&](const glm::vec3& axisIn) {
						auto axisTransformed = ofxCeres::VectorMath::applyTransform(cameraTransform, axisIn) - cameraPosition;
						axisTransformed.y = 0.0f; // flatten in y;
						return axisTransformed / glm::length(axisTransformed); // normalise
					};

					// Calculate the x and z axes (we ignore y axis)
					auto x_axis = findAxis({ 1, 0, 0 });
					auto z_axis = findAxis({ 0, 0, 1 });

					auto cameraToLight = this->parameters.lightPosition.x.get() * x_axis
						+ glm::vec3(0, this->parameters.lightPosition.y.get(), 0)
						+ this->parameters.lightPosition.z.get() * z_axis;

					return cameraPosition + cameraToLight;
				}

				//--------
				void
					DroneLightInMirror::calculateResiduals()
				{
					this->throwIfMissingAConnection<Heliostats2>();
					auto heliostatsNode = this->getInput<Heliostats2>();
					auto activeHeliostats = heliostatsNode->getHeliostats();

					auto captures = this->captures.getSelection();

					for (auto& capture : captures) {
						float totalResidual = 0.0f;

						for (auto& heliostatCapture : capture->heliostatCaptures) {
							shared_ptr<Heliostats2::Heliostat> heliostat;
							for (auto activeHeliostat : activeHeliostats) {
								if (activeHeliostat->parameters.name.get() == heliostatCapture.heliostatName) {
									heliostat = activeHeliostat;
									break;
								}
							}
							if (!heliostat) {
								// couldn't find the heliostat
								continue;
							}

							auto rayResidual = Solvers::HeliostatActionModel::Calibrator::getResidual(heliostatCapture.cameraRay
								, capture->lightPosition
								, heliostatCapture.axis1ServoPosition
								, heliostatCapture.axis2ServoPosition
								, heliostat->parameters.servo1.angleOffset.get()
								, heliostat->parameters.servo2.angleOffset.get()
								, heliostat->getHeliostatActionModelParameters()
								, heliostat->parameters.diameter.get());
							
							rayResidual = sqrt(rayResidual);

							totalResidual += rayResidual;
							heliostatCapture.residual = rayResidual;

							// Create the board plane
							Solvers::HeliostatActionModel::AxisAngles<float> axisAngles{
								Solvers::HeliostatActionModel::positionToAngle<float>(heliostatCapture.axis1ServoPosition
									, heliostat->parameters.hamParameters.axis1.polynomial.get()
									, heliostatCapture.axis1AngleOffest)
								, Solvers::HeliostatActionModel::positionToAngle<float>(heliostatCapture.axis2ServoPosition
									, heliostat->parameters.hamParameters.axis2.polynomial.get()
									, heliostatCapture.axis2AngleOffest)
							};

							// Get mirror plane
							auto mirrorPlane = Solvers::HeliostatActionModel::getMirrorPlane(axisAngles
								, heliostat->getHeliostatActionModelParameters());
							heliostatCapture.mirrorCenterEstimated = mirrorPlane.center;
							heliostatCapture.mirrorNormalEstimated = mirrorPlane.normal;

							// Crop the camera ray to intersect mirror
							auto & cameraRay = heliostatCapture.cameraRay;
							ofxCeres::Models::Ray<float> cameraRay_;
							{
								cameraRay_.s = cameraRay.s;
								cameraRay_.t = cameraRay.t;
							}
							{
								auto positionHitsMirror = mirrorPlane.intersect(cameraRay_);
								cameraRay.setEnd(positionHitsMirror);
							}

							// Calculate the reflections
							{
								auto reflectedRay_ = mirrorPlane.reflect(cameraRay_);
								heliostatCapture.reflectedRayEstimated = heliostatCapture.cameraRay; // for color, etc
								heliostatCapture.reflectedRayEstimated.s = reflectedRay_.s;
								heliostatCapture.reflectedRayEstimated.t = reflectedRay_.t;

								auto length = glm::length(capture->lightPosition - heliostatCapture.reflectedRayEstimated.s);
								heliostatCapture.reflectedRayEstimated.t *= length / glm::length(heliostatCapture.reflectedRayEstimated.t);
							}
						}

						capture->meanResidual = totalResidual / (float)capture->heliostatCaptures.size();
					}
				}

				//----------
				ofxCeres::SolverSettings
					DroneLightInMirror::getNavigateSolverSettings() const
				{
					auto solverSettings = ofxRulr::Solvers::HeliostatActionModel::Navigator::defaultSolverSettings();

					solverSettings.options.max_num_iterations = this->parameters.navigate.maxIterations.get();
					solverSettings.printReport = this->parameters.navigate.printReport.get();
					solverSettings.options.minimizer_progress_to_stdout = this->parameters.navigate.printReport.get();

					return solverSettings;
				}

				//----------
				ofxCeres::SolverSettings
					DroneLightInMirror::getCalibrateSolverSettings() const
				{
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
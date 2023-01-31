#include "pch_Plugin_Scrap.h"
#include "Calibrate.h"
#include "Lasers.h"

namespace ofxRulr {
	namespace Nodes {
		namespace AnotherMoon {
#pragma mark ImagePath
			//----------
			void
				Calibrate::ImagePath::serialize(nlohmann::json& json)
			{
				Utils::serialize(json, "pathOnCamera", this->pathOnCamera);
				Utils::serialize(json, "localCopy", this->localCopy);
			}

			//----------
			void
				Calibrate::ImagePath::deserialize(const nlohmann::json& json)
			{
				Utils::deserialize(json, "pathOnCamera", this->pathOnCamera);
				Utils::deserialize(json, "localCopy", this->localCopy);
			}

#pragma mark BeamCapture
			//----------
			Calibrate::BeamCapture::BeamCapture()
			{
				RULR_SERIALIZE_LISTENERS;
			}

			//----------
			string
				Calibrate::BeamCapture::getDisplayString() const
			{
				stringstream ss;
				ss << "residual: " << this->residual << ". Line : " << this->line.s << "->" << this->line.t;
				return ss.str();
			}

			//----------
			void
				Calibrate::BeamCapture::serialize(nlohmann::json& json)
			{
				Utils::serialize(json, "projectionPoint", this->projectionPoint);
				Utils::serialize(json, "residual", this->residual);
				Utils::serialize(json, "isOffset", this->isOffset);

				this->onImage.serialize(json["onImage"]);
				this->offImage.serialize(json["offImage"]);

				this->line.serialize(json["line"]);

			}

			//----------
			void
				Calibrate::BeamCapture::deserialize(const nlohmann::json& json)
			{
				// legacy encoding
				{
					if (json.contains("imagePoint")) {
						Utils::deserialize(json, "imagePoint", this->projectionPoint);
					}

					if (json.contains("urlOnImage")) {
						this->onImage.pathOnCamera = json["urlOnImage"].get<string>();
					}
					if (json.contains("urlOffImage")) {
						this->offImage.pathOnCamera = json["urlOffImage"].get<string>();
					}
				}

				Utils::deserialize(json, "projectionPoint", this->projectionPoint);
				Utils::deserialize(json, "residual", this->residual);
				Utils::deserialize(json, "isOffset", this->isOffset);

				if (json.contains("onImage")) {
					this->onImage.deserialize(json["onImage"]);
				}
				if (json.contains("offImage")) {
					this->offImage.deserialize(json["offImage"]);
				}

				if (json.contains("line")) {
					this->line.deserialize(json["line"]);
				}
			}

			//----------
			ofxCvGui::ElementPtr
				Calibrate::BeamCapture::getDataDisplay()
			{
				auto element = ofxCvGui::makeElement();

				vector<ofxCvGui::ElementPtr> widgets;

				widgets.push_back(make_shared<ofxCvGui::Widgets::LiveValue<string>>("", [this]() {
					return this->getDisplayString();
					}));
				{
					auto button = make_shared<ofxCvGui::Widgets::Toggle>("Beam capture"
						, [this]() {
							if (this->parentSelection) {
								return this->parentSelection->isSelected(this);
							}
							else {
								return false;
							}
						}
						, [this](bool value) {
							if (this->parentSelection) {
								this->parentSelection->select(this);
							}
							else {
								ofLogError() << "EditSelection not initialised";
							}
						});
					button->setDrawGlyph(u8"\uf03a");
					widgets.push_back(button);
				}


				for (auto& widget : widgets) {
					element->addChild(widget);
				}

				element->onBoundsChange += [this, widgets](ofxCvGui::BoundsChangeArguments& args) {
					auto bounds = args.localBounds;
					bounds.height = 40.0f;

					for (auto& widget : widgets) {
						widget->setBounds(bounds);
						bounds.y += bounds.height;
					}
				};

				element->setHeight(widgets.size() * 40 + 10);

				return element;
			}

#pragma mark LaserCapture
			//----------
			Calibrate::LaserCapture::LaserCapture()
			{
				RULR_SERIALIZE_LISTENERS;
			}

			//----------
			string
				Calibrate::LaserCapture::getDisplayString() const
			{
				stringstream ss;
				ss << this->beamCaptures.size() << " beam captures";
				return ss.str();
			}

			//----------
			void
				Calibrate::LaserCapture::serialize(nlohmann::json& json)
			{
				this->beamCaptures.serialize(json["beamCaptures"]);
				Utils::serialize(json, "serialNumber", this->serialNumber);
				Utils::serialize(json, "directory", this->directory);
				Utils::serialize(json, "imagePointInCamera", this->imagePointInCamera);
				{
					Utils::serialize(json["linesWithCommonPointSolveResult"], "residual", this->linesWithCommonPointSolveResult.residual);
					Utils::serialize(json["linesWithCommonPointSolveResult"], "success", this->linesWithCommonPointSolveResult.success);
				}
				Utils::serialize(json, "parameters", this->parameters);
			}

			//----------
			void
				Calibrate::LaserCapture::deserialize(const nlohmann::json& json)
			{
				if (json.contains("beamCaptures")) {
					this->beamCaptures.deserialize(json["beamCaptures"]);
				}
				Utils::deserialize(json, "serialNumber", this->serialNumber);
				Utils::deserialize(json, "directory", this->directory);
				Utils::deserialize(json, "imagePointInCamera", this->imagePointInCamera);

				if (json.contains("linesWithCommonPointSolveResult")) {
					Utils::deserialize(json["linesWithCommonPointSolveResult"], "residual", this->linesWithCommonPointSolveResult.residual);
					Utils::deserialize(json["linesWithCommonPointSolveResult"], "success", this->linesWithCommonPointSolveResult.success);
				}

				Utils::deserialize(json, "parameters", this->parameters);

				auto beamCaptures = this->beamCaptures.getAllCaptures();
				for (auto beamCapture : beamCaptures) {
					beamCapture->parentSelection = &this->ourSelection;
				}
			}

			//----------
			void
				Calibrate::LaserCapture::drawWorldStage(const DrawArguments& args)
			{
				auto drawRays = ofxRulr::isActive(args.nodeForSelection, args.drawParameters.beamCaptures.rays.get());
				auto drawRayIndices = ofxRulr::isActive(args.nodeForSelection, args.drawParameters.beamCaptures.rayIndices.get());

				// Draw beams
				if (args.lasers && (drawRays || drawRayIndices)) {
					auto lasers = args.lasers->getLasersSelected();

					// Find corresponding laser
					shared_ptr<Laser> laser;
					for (const auto& it : lasers) {
						if (it->parameters.serialNumber.get() == this->serialNumber) {
							laser = it;
							break;
						}
					}

					// Draw beam rays
					if (laser) {
						auto laserModel = laser->getModel();
						auto beamCaptures = this->beamCaptures.getSelection();

						ofMesh lines;

						size_t beamIndex = 0;
						for (auto beamCapture : beamCaptures) {
							auto rayModel = laserModel.castRayWorldSpace(beamCapture->projectionPoint);

							if (drawRays) {
								lines.addVertex(rayModel.s);
								lines.addVertex(rayModel.s + rayModel.t);

								lines.addColor(beamCapture->color.get());
								lines.addColor(beamCapture->color.get());
							}

							if (drawRayIndices) {

								ofxCvGui::Utils::drawTextAnnotation(ofToString(beamIndex)
									, rayModel.s + rayModel.t
									, beamCapture->color);
							}

							beamIndex++;
						}

						if (drawRays) {
							lines.setMode(ofPrimitiveMode::OF_PRIMITIVE_LINES);
							lines.draw();
						}
					}
				}
			}

			//----------
			ofxCvGui::ElementPtr
				Calibrate::LaserCapture::getDataDisplay()
			{
				auto element = ofxCvGui::makeElement();

				vector<ofxCvGui::ElementPtr> widgets;

				widgets.push_back(make_shared<ofxCvGui::Widgets::LiveValue<string>>("Serial #" + ofToString(this->serialNumber), [this]() {
					return this->getDisplayString();
					}));
				{
					auto button = make_shared<ofxCvGui::Widgets::Toggle>("Beams in laser"
						, [this]() {
							if (this->parentSelection) {
								return this->parentSelection->isSelected(this);
							}
							else {
								return false;
							}
						}
						, [this](bool value) {
							if (this->parentSelection) {
								this->parentSelection->select(this);
							}
							else {
								ofLogError() << "EditSelection not initialised";
							}
						});
					button->setDrawGlyph(u8"\uf03a");
					widgets.push_back(button);
				}


				for (auto& widget : widgets) {
					element->addChild(widget);
				}

				element->onBoundsChange += [this, widgets](ofxCvGui::BoundsChangeArguments& args) {
					auto bounds = args.localBounds;
					bounds.height = 40.0f;

					for (auto& widget : widgets) {
						widget->setBounds(bounds);
						bounds.y += bounds.height;
					}
				};

				element->setHeight(widgets.size() * 40 + 10);

				return element;
			}

#pragma mark CameraCapture
			//----------
			Calibrate::CameraCapture::CameraCapture()
			{
				RULR_SERIALIZE_LISTENERS;
				this->cameraTransform->init();
			}

			//----------
			string
				Calibrate::CameraCapture::getName() const
			{
				return this->timeString;
			}

			//----------
			string
				Calibrate::CameraCapture::getDisplayString() const
			{
				stringstream ss;
				ss << this->laserCaptures.size() << " laser captures";
				return ss.str();
			}

			//----------
			void
				Calibrate::CameraCapture::serialize(nlohmann::json& json)
			{
				this->laserCaptures.serialize(json["laserCaptures"]);
				Utils::serialize(json, "directory", this->directory);
				this->cameraTransform->serialize(json["cameraTransform"]);

			}

			//----------
			void
				Calibrate::CameraCapture::deserialize(const nlohmann::json& json)
			{
				if (json.contains("laserCaptures")) {
					this->laserCaptures.deserialize(json["laserCaptures"]);
				}

				Utils::deserialize(json, "directory", this->directory);
				
				auto laserCaptures = this->laserCaptures.getAllCaptures();
				for (auto capture : laserCaptures) {
					capture->parentSelection = &this->ourSelection;
				}

				if (json.contains("cameraTransform")) {
					this->cameraTransform->deserialize(json["cameraTransform"]);
				}
			}

			//----------
			void
				Calibrate::CameraCapture::update()
			{
				// Set the name for in the WorldStage
				this->cameraTransform->setName(this->getName());
				this->cameraTransform->setColor(this->color);
			}

			//----------
			void
				Calibrate::CameraCapture::drawWorldStage(const DrawArguments& args)
			{
				this->cameraTransform->drawWorldStage();
				if(ofxRulr::isActive(args.nodeForSelection, args.drawParameters.cameraCaptures.cameras)) {
					ofPushMatrix();
					{
						ofMultMatrix(this->cameraTransform->getTransform());
						this->drawObjectSpace(args);
					}
					ofPopMatrix();
				}

				auto laserCaptures = this->laserCaptures.getSelection();
				for (auto laserCapture : laserCaptures) {
					laserCapture->drawWorldStage(args);
				}
			}

			//----------
			void
				Calibrate::CameraCapture::drawObjectSpace(const DrawArguments& args)
			{
				if (args.camera) {
					auto view = args.camera->getViewInObjectSpace();
					view.color = this->color;
					view.draw();
				}
			}

			//----------
			ofxCvGui::ElementPtr
				Calibrate::CameraCapture::getDataDisplay()
			{
				auto element = ofxCvGui::makeElement();

				auto stack = make_shared<ofxCvGui::Widgets::HorizontalStack>();

				stack->add(make_shared<ofxCvGui::Widgets::LiveValue<size_t>>("Laser capture count", [this]() {
					return this->laserCaptures.size();
					}));

				// Select lasers in camera
				{
					auto button = make_shared<ofxCvGui::Widgets::Toggle>("Lasers in camera"
						, [this]() {
							if (this->parentSelection) {
								return this->parentSelection->isSelected(this);
							}
							else {
								return false;
							}
						}
						, [this](bool value) {
							if (this->parentSelection) {
								this->parentSelection->select(this);
							}
							else {
								ofLogError() << "EditSelection not initialised";
							}
						});
					button->setDrawGlyph(u8"\uf03a");
					stack->add(button);
				}

				// Select the rigidBody
				{
					auto button = make_shared<ofxCvGui::Widgets::Toggle>("Camera transform"
						, [this]() {
							return this->cameraTransform->isBeingInspected();
						}
						, [this](bool value) {
							if (value) {
								ofxCvGui::inspect(this->cameraTransform);
							}
						});
					button->setDrawable([this](ofxCvGui::DrawArguments& args) {
						ofRectangle bounds;
						bounds.setFromCenter(args.localBounds.getCenter(), 32, 32);
						this->cameraTransform->getIcon()->draw(bounds);
						});
					stack->add(button);
				}


				return stack;
			}

#pragma mark Calibrate
			//----------
			Calibrate::Calibrate()
			{
				RULR_NODE_INIT_LISTENER;
			}

			//----------
			string
				Calibrate::getTypeName() const
			{
				return "AnotherMoon::Calibrate";
			}

			//----------
			void
				Calibrate::init()
			{
				this->addInput<Lasers>();
				this->addInput<Item::Camera>("Calibrated camera");

				RULR_NODE_UPDATE_LISTENER;
				RULR_NODE_INSPECTOR_LISTENER;
				RULR_NODE_SERIALIZATION_LISTENERS;
				RULR_NODE_DRAW_WORLD_LISTENER;

				this->manageParameters(this->parameters);

				{
					// Make the panel
					auto panel = ofxCvGui::Panels::makeWidgets();
					panel->addTitle("Camera positions:", ofxCvGui::Widgets::Title::Level::H3);
					this->cameraCaptures.populateWidgets(panel);
					panel->addButton("Select all children", [this]() {
						this->selectAllChildren();
						});
					panel->addButton("Select children with solved lines", [this]() {
						this->selectChildrenWithSolvedLines();
						})->addToolTip("Has solution and not marked bad");
					panel->addButton("Deselect children with bad lines", [this]() {
						this->deselectChildrenWithBadLines();
						});
					this->panel = panel;
				}

				this->cameraCaptures.onDeserialize += [this](const nlohmann::json&) {
					auto captures = this->cameraCaptures.getAllCaptures();
					for (auto capture : captures) {
						capture->parentSelection = &this->cameraEditSelection;
					}
				};
			}

			//----------
			void
				Calibrate::update()
			{
				// they need updating to pull the name
				{
					auto cameraCaptures = this->cameraCaptures.getAllCaptures();
					for (auto cameraCapture : cameraCaptures) {
						cameraCapture->update();
					}
				}
			}

			//----------
			void
				Calibrate::populateInspector(ofxCvGui::InspectArguments& inspectArgs)
			{
				auto inspector = inspectArgs.inspector;

				inspector->addButton("Capture", [this]() {
					try {
						this->capture();
					}
					RULR_CATCH_ALL_TO_WARNING;
					}, ' ');

				inspector->addButton("Offset beam captures", [this]() {
					try {
						this->offsetBeamCaptures();
					}
					RULR_CATCH_ALL_TO_ALERT;
					});


				inspector->addTitle("Calibrate", ofxCvGui::Widgets::Title::Level::H2);
				{
					inspector->addButton("Lines", [this]() {
						try {
							this->calibrateLines();
						}
						RULR_CATCH_ALL_TO_ALERT;
						}, '1');

					inspector->addButton("Initial cameras", [this]() {
						try {
							this->calibrateInitialCameras();
						}
						RULR_CATCH_ALL_TO_ALERT;
						}, '2');

					inspector->addButton("Bundle adjust points", [this]() {
						try {
							this->calibrateBundleAdjustPoints();
						}
						RULR_CATCH_ALL_TO_ALERT;
						}, '3');

					inspector->addButton("Bundle adjust lasers", [this]() {
						try {
							this->calibrateBundleAdjustLasers(true);
						}
						RULR_CATCH_ALL_TO_ALERT;
						}, '4');

					inspector->addButton("Calculate laser residuals", [this]() {
						try {
							this->calibrateBundleAdjustLasers(false);
						}
						RULR_CATCH_ALL_TO_ALERT;
						}, '5');
				}

				inspector->addTitle("Prune", ofxCvGui::Widgets::Title::Level::H2);
				{
					inspector->addButton("Beam captures by residual", [this]() {
						try {
							auto text = ofSystemTextBoxDialog("Max residual");
							if (!text.empty()) {
								auto maxResidual = ofToFloat(text);
								this->pruneBeamCapturesByResidual(maxResidual);
							}
						}
						RULR_CATCH_ALL_TO_ALERT;
						});
				}

			}

			//----------
			void 
				Calibrate::serialize(nlohmann::json& json)
			{
				this->cameraCaptures.serialize(json["cameraCaptures"]);
			}

			//----------
			void 
				Calibrate::deserialize(const nlohmann::json& json)
			{
				if (json.contains("cameraCaptures")) {
					this->cameraCaptures.deserialize(json["cameraCaptures"]);
				}
				auto cameraCaptures = this->cameraCaptures.getAllCaptures();
				for (auto capture : cameraCaptures) {
					capture->parentSelection = &this->cameraEditSelection;
				}
			}


			//----------
			ofxCvGui::PanelPtr
				Calibrate::getPanel()
			{
				return this->panel;
			}

			//----------
			void
				Calibrate::drawWorldStage()
			{
				//prep draw parameters
				DrawArguments drawArgs{
					this
					, this->getInput<Item::Camera>()
					, this->getInput<Lasers>()
					, this->parameters.draw
				};

				auto cameraCaptures = this->cameraCaptures.getSelection();
				for (auto cameraCapture : cameraCaptures) {
					cameraCapture->drawWorldStage(drawArgs);
				}
			}

			//----------
			void
				Calibrate::capture()
			{
				// This is because we have an issue disposing
				static vector<shared_ptr<CameraCapture>> allCameraCaptures;
				static vector<shared_ptr<LaserCapture>> allLaserCaptures;
				static vector<shared_ptr<BeamCapture>> allBeamCaptures;

				this->throwIfMissingAConnection<Lasers>();
				auto lasersNode = this->getInput<Lasers>();
				auto allLasers = lasersNode->getLasersSelected();

				auto calibrationImagePoints = this->getCalibrationImagePoints();

				bool dryRun = this->parameters.capture.dryRun.enabled.get();

				Utils::ScopedProcess scopedProcess("Calibrate", false, allLasers.size());

				auto cameraCapture = make_shared<CameraCapture>();
				allCameraCaptures.push_back(cameraCapture);
				cameraCapture->parentSelection = &this->cameraEditSelection;

				auto tryNtimes = [this](const function<future<void>()>& getFuture) {
					return std::async([=]() {
						bool success = false;
						for (int t = 0; t < this->parameters.capture.messageTransmitTries.get(); t++) {
							try {
								auto future = getFuture();
								future.get();
								success = true;
								break;
							}
							catch (const std::exception& e) {
								ofLogWarning("Message send failed") << e.what();
							}
						}
						if (!success) {
							throw(Exception("Max tries reached sending message"));
						}
						});

				};

				// iterate through lasers, send beams and capture
				for (const auto & laser : allLasers) {
					Utils::ScopedProcess scopedProcessLaser("Laser s#" + ofToString(laser->parameters.serialNumber.get()) + "p#" + ofToString(laser->parameters.positionIndex.get()), false);

					// Gather other lasers
					auto otherLasers = allLasers;
					otherLasers.erase(std::remove(otherLasers.begin(), otherLasers.end(), laser), otherLasers.end());
					
					// Set all others to laserStateForOthers
					{
						Utils::ScopedProcess scopedProcessOtherLasers("Setting state on other lasers", false);

						vector<std::future<void>> actions;
						switch (this->parameters.capture.laserStateForOthers.get().get()) {
						case LaserState::Shutdown:
							for (auto laser : otherLasers) {
								laser->shutdown();
								laser->parameters.deviceState.state.set(Laser::State::Shutdown);
								actions.push_back(tryNtimes([=]() {
									return laser->pushState();
									}));
							}
							break;
						case LaserState::Standby:
							for (auto laser : otherLasers) {
								laser->parameters.deviceState.state.set(Laser::State::Standby);
								actions.push_back(tryNtimes([=]() {
									return laser->pushState();
									}));
							}
							break;
						case LaserState::Run:
							for (auto laser : otherLasers) {
								laser->parameters.deviceState.state.set(Laser::State::Run);
								actions.push_back(tryNtimes([=]() {
									return laser->pushState();
									}));
							}
							break;
						case LaserState::TestPattern:
							lasersNode->sendTestImageTo(otherLasers);
							break;
						}
						for (auto& action : actions) {
							action.get();
						}
					}

					// Turn this laser on
					{
						Utils::ScopedProcess scopedProcessOtherLasers("Setting state on this laser", false);

						laser->parameters.deviceState.state.set(Laser::State::Run);
						tryNtimes([=]() {
							return laser->pushState();
							}).get();
					}

					// Create a new laserCapture
					auto laserCapture = make_shared<LaserCapture>();
					allLaserCaptures.push_back(laserCapture);
					{
						laserCapture->serialNumber = laser->parameters.serialNumber.get();
						laserCapture->parentSelection = &cameraCapture->ourSelection;
					}

					// Iterate thorugh the calibration image points
					{
						Utils::ScopedProcess scopedProcessImagePoints("Image points"
							, false
							, calibrationImagePoints.size());
						for (const auto& calibrationImagePoint : calibrationImagePoints) {
							try {
								auto beamCapture = make_shared<BeamCapture>();
								allBeamCaptures.push_back(beamCapture);

								Utils::ScopedProcess scopedProcessImagePoint(ofToString(calibrationImagePoint));

								beamCapture->projectionPoint = calibrationImagePoint;
								beamCapture->parentSelection = &laserCapture->ourSelection;

								//Legacy
								{
									// Originally we were offsetting when rendering
									// Now we send raw beams out without offset during calibration
									beamCapture->isOffset = false;
								}

								// Capture background pass
								{
									// Zero brightness
									{
										laser->parameters.deviceState.projection.color.red.set(0);
										laser->parameters.deviceState.projection.color.green.set(0);
										laser->parameters.deviceState.projection.color.blue.set(0);
										tryNtimes([=]() {
											return laser->pushColor();
											}).get();
									}

									// Standby
									if (this->parameters.capture.standbyForBackgroundCapture) {
										auto state = laser->parameters.deviceState.state.get();
										state.set(Laser::State::Standby);
										laser->parameters.deviceState.state.set(state);
										tryNtimes([=]() {
											return laser->pushState();
											}).get();
									}

									this->waitForDelay();

									if (!dryRun) {
										beamCapture->offImage.pathOnCamera = this->captureToURL();
									}
								}

								// Capture foreground pass (positive image)
								{
									// Draw the beam
									tryNtimes([=]() {
										return laser->drawCalibrationBeam(calibrationImagePoint);
										}).get();

									// Full brightness
									{
										laser->parameters.deviceState.projection.color.red.set(1);
										laser->parameters.deviceState.projection.color.green.set(1);
										laser->parameters.deviceState.projection.color.blue.set(1);
										tryNtimes([=]() {
											return laser->pushColor();
											}).get();
									}

									// Remove standby
									if (this->parameters.capture.standbyForBackgroundCapture) {
										auto state = laser->parameters.deviceState.state.get();
										state.set(Laser::State::Run);
										laser->parameters.deviceState.state.set(state);
										tryNtimes([=]() {
											return laser->pushState();
											}).get();
									}

									this->waitForDelay();

									if (!dryRun) {
										beamCapture->onImage.pathOnCamera = this->captureToURL();
									}
								}

								// Save the data
								if (!dryRun) {
									laserCapture->beamCaptures.add(beamCapture);
								}

								scopedProcessImagePoint.end();
							}
							RULR_CATCH_ALL_TO({
								if (this->parameters.capture.continueOnFail) {
									ofLogError(this->getTypeName()) << e.what();
								}
								else {
									throw e;
								}
								});
						}
					}

					if (laserCapture->beamCaptures.size() > 0) {
						cameraCapture->laserCaptures.add(laserCapture);
					}
				}

				// Set all to laserStateForOthers
				if (this->parameters.capture.laserStateForOthers.get() == LaserState::TestPattern) {
					lasersNode->sendTestImageTo(allLasers);
				}
				else {
					vector<std::future<void>> actions;
					for (const auto& laser : allLasers) {
						laser->parameters.deviceState.state.set(Laser::State::Standby);
						actions.push_back(tryNtimes([=]() {
							return laser->pushState();
							}));
					}
					for (auto& action : actions) {
						action.get();
					}
				}

				if (!dryRun) {
					this->cameraCaptures.add(cameraCapture);
				}
			}

			//----------
			const Utils::CaptureSet<Calibrate::CameraCapture> &
				Calibrate::getCameraCaptures()
			{
				return this->cameraCaptures;
			}

			//----------
			filesystem::path
				Calibrate::getLocalCopyPath(const ImagePath& imagePath) const
			{
				if (imagePath.localCopy.empty()) {
					// render an image path based on a local directory
					auto url = imagePath.pathOnCamera;
					auto splitPath = ofSplitString(url, "/");
					auto filename = splitPath.back();
					auto localPath = filesystem::path(this->parameters.lineFinder.localDirectory.get());
					if (localPath.empty()) {
						throw(ofxRulr::Exception("Local path is empty"));
					}
					if (!filesystem::is_directory(localPath)) {
						localPath = localPath.parent_path();
					}
					auto fullFilePath = localPath / filename;
					return fullFilePath;
				}
				else {
					// use the image path that's stored
					return imagePath.localCopy;
				}
			}

			//----------
			void
				Calibrate::deselectLasersWithNoData(size_t minimumCameraCaptureCount)
			{
				this->throwIfMissingAConnection<Lasers>();
				auto lasersNode = this->getInput<Lasers>();
				auto lasers = lasersNode->getLasersSelected();
				auto cameraCaptures = this->cameraCaptures.getSelection();

				for (auto laser : lasers) {
					size_t seenInCountViews = 0;
					for (auto cameraCapture : cameraCaptures) {
						auto laserCaptures = cameraCapture->laserCaptures.getSelection();

						// Deselect any without children
						for (auto laserCapture : laserCaptures) {
							// Check if it has any beamCaptures
							if (laserCapture->beamCaptures.getSelection().size() == 0) {
								laserCapture->setSelected(false);
							}

							// Count if selected and is this laser
							if (laserCapture->isSelected()
								&& laserCapture->serialNumber == laser->parameters.serialNumber.get()) {
								seenInCountViews++;
							}
						}
					}
					if (seenInCountViews < minimumCameraCaptureCount) {
						laser->setSelected(false);
					}
				}
			}

			//----------
			void
				Calibrate::offsetBeamCaptures()
			{
				// Perform for all data (ignore selection)

				this->throwIfMissingAConnection<Lasers>();

				auto lasers = this->getInput<Lasers>()->getLasersAll();

				auto cameraCaptures = this->cameraCaptures.getAllCaptures();
				for(auto cameraCapture : cameraCaptures) {
					auto laserCaptures = cameraCapture->laserCaptures.getAllCaptures();
					for (auto laserCapture : laserCaptures) {
						auto beamCaptures = laserCapture->beamCaptures.getAllCaptures();

						// find laser
						shared_ptr<Laser> laser;
						for (auto it : lasers) {
							if (it->parameters.serialNumber.get() == laserCapture->serialNumber) {
								laser = it;
								break;
							}
						}
						if (!laser) {
							// ignore laserCapture if we don't have any laser for it
							continue;
						}

						// get center offset
						const auto & centerOffset = laser->parameters.intrinsics.centerOffset.get();

						// Correct the beam captures
						size_t beamCaptureIndex = 0;
						for (auto beamCapture : beamCaptures) {
							if (beamCapture->isOffset) {
								beamCapture->projectionPoint += centerOffset;
								if (beamCapture->projectionPoint.x < -1
									|| beamCapture->projectionPoint.x > 1
									|| beamCapture->projectionPoint.y < -1
									|| beamCapture->projectionPoint.y > 1) {
									cout << cameraCapture->getTimeString()
										<< "::" << laserCapture->serialNumber
										<< "::" << beamCaptureIndex
										<< " is outside range : " << beamCapture->projectionPoint
										<< endl;

									beamCapture->projectionPoint.x = ofClamp(beamCapture->projectionPoint.x, -1, 1);
									beamCapture->projectionPoint.y = ofClamp(beamCapture->projectionPoint.y, -1, 1);
								}

								beamCapture->isOffset = false;
							}
							beamCaptureIndex++;
						}
					}
				}
			}

			//----------
			void
				Calibrate::pruneBeamCapturesByResidual(float maxResidual)
			{
				auto cameraCaptures = this->cameraCaptures.getSelection();
				for (auto cameraCapture : cameraCaptures) {
					auto laserCaptures = cameraCapture->laserCaptures.getSelection();
					for (auto laserCapture : laserCaptures) {
						auto beamCaptures = laserCapture->beamCaptures.getSelection();
						for (auto beamCapture : beamCaptures) {
							if (beamCapture->residual > maxResidual) {
								beamCapture->setSelected(false);
							}
						}
					}
				}
			}

			//----------
			vector<glm::vec2>
				Calibrate::getCalibrationImagePoints() const
			{
				auto resolution = this->parameters.capture.imagePoints.resolution.get();
				auto radius = this->parameters.capture.imagePoints.radius.get();

				vector<glm::vec2> imagePoints;

				for (int j = 0; j < resolution; j++) {
					for (int i = 0; i < resolution; i++) {
						imagePoints.emplace_back(
							ofMap(i, 0, resolution - 1, -radius, radius)
							, ofMap(j, 0, resolution - 1, -radius, radius)
						);
					}
				}

				return imagePoints;
			}

			//----------
			void
				Calibrate::waitForDelay() const
			{
				std::this_thread::sleep_for(chrono::microseconds((int) (1000000.0f * this->parameters.capture.outputDelay.get())));
			}

			//----------
			string
				Calibrate::getBaseCameraURL() const
			{
				return "http://" + this->parameters.capture.remoteCamera.hostname.get() + ":8080/ccapi/ver100/";
			}

			//----------
			string
				Calibrate::captureToURL()
			{
				// Take the photo
				this->takePhoto();

				// Poll until the photo is available
				auto timeStart = ofGetElapsedTimef();
				do {
					auto newFiles = this->pollNewCameraFiles();
					if (newFiles.empty()) {
						ofSleepMillis(100);
						continue;
					}

					// This might happen if we didn't flush incoming photos, or if we are capturing both RAW and JPG
					if (newFiles.size() > 1) {
						throw(ofxRulr::Exception("More than one photo returned when taking photo"));
					}

					// This is good - we got one file, return it as URL
					return newFiles[0];
				} while (ofGetElapsedTimef() - timeStart < this->parameters.capture.remoteCamera.captureTimeout.get());

				throw(ofxRulr::Exception("Calibrate::captureToURL Timed out"));
			}

			//----------
			void
				Calibrate::takePhoto()
			{
				ofHttpRequest request;
				{
					request.method = ofHttpRequest::POST;
					request.url = this->getBaseCameraURL() + "shooting/control/shutterbutton";

					nlohmann::json requestData;
					requestData["af"] = false;
					request.body = requestData.dump();
				}

				auto response = this->urlLoader.handleRequest(request);

				if (response.status != 200) {
					throw(ofxRulr::Exception("Take photo failed : " + (string)response.data));
				}
			}

			//----------
			vector<string>
				Calibrate::pollNewCameraFiles()
			{
				// This function makes a request to a supported Canon EOS camera over REST 

				ofHttpRequest request;
				{
					request.method = ofHttpRequest::GET;
					request.url = this->getBaseCameraURL() + "event/polling";
				}

				auto response = this->urlLoader.handleRequest(request);

				if (response.status != 200) {
					throw(ofxRulr::Exception("Couldn't poll camera : " + (string)response.data));
				}

				// Look through the response from the camera
				auto json = nlohmann::json::parse(response.data);
				vector<string> results;
				if (json.contains("addedcontents")) {
					for (const auto& filenameJson : json["addedcontents"]) {
						results.push_back(filenameJson.get<string>());
					}
				}

				return results;
			}

			//----------
			cv::Mat
				Calibrate::fetchImage(const ImagePath& imagePath) const
			{
				if (this->parameters.lineFinder.amplifyBlue.get()) {
					return this->fetchImageAmplifyBlue(imagePath);
				}

				auto flags = cv::IMREAD_GRAYSCALE;

				switch (this->parameters.lineFinder.imageFileSource.get()) {
					case ImageFileSource::Local:
					{
						auto fullFilePath = this->getLocalCopyPath(imagePath);

						auto image = cv::imread(fullFilePath.string(), flags);
						if (image.empty()) {
							throw(Exception("Failed to load image at " + fullFilePath.string()));
						}
						return image;
					}
					case ImageFileSource::Camera:
					{
						auto response = ofLoadURL("http://" + this->parameters.capture.remoteCamera.hostname.get() + ":8080" + imagePath.pathOnCamera);
						if (response.status == 200) {
							auto data = response.data.getData();
							vector<unsigned char> buffer(data, data + response.data.size());
							return cv::imdecode(buffer, flags);
						}
						else {
							throw(Exception("Failed to load image from camera : " + (string) response.data));
						}
					}
					default:
						throw(Exception("Cannot fetchImage. Method is undefined"));
				}
			}

			//----------
			cv::Mat
				Calibrate::fetchImageAmplifyBlue(const ImagePath& imagePath) const
			{
				switch (this->parameters.lineFinder.imageFileSource.get()) {
				case ImageFileSource::Local:
				{
					auto fullFilePath = this->getLocalCopyPath(imagePath);

					auto image = cv::imread(fullFilePath.string());
					if (image.empty()) {
						throw(Exception("Failed to load image at " + fullFilePath.string()));
					}
					
					cv::Mat colorPlanes[3];
					cv::Mat blueMinusOthers;
					cv::split(image, colorPlanes);
					cv::subtract(colorPlanes[0], colorPlanes[1], blueMinusOthers);
					cv::subtract(blueMinusOthers, colorPlanes[2], blueMinusOthers);
					blueMinusOthers = 4 * blueMinusOthers;
					return blueMinusOthers;
				}
				default:
					throw(Exception("Cannot fetchImage. Method is not implemented by fetchImageAmplifyBlue"));
				}
			}

			//----------
			void
				Calibrate::configureSolverSettings(ofxCeres::SolverSettings& solverSettings, const SolverSettings& parameters) const
			{
				solverSettings.printReport = parameters.printOutput;
				solverSettings.options.minimizer_progress_to_stdout = parameters.printOutput;
				solverSettings.options.max_num_iterations = parameters.maxIterations;
				solverSettings.options.num_threads = parameters.threads;
				solverSettings.options.function_tolerance = parameters.functionTolerance.get();
				solverSettings.options.parameter_tolerance = parameters.parameterTolerance.get();
			}

			//----------
			void
				Calibrate::selectAllChildren()
			{
				auto cameraCaptures = this->cameraCaptures.getSelection();
				for (auto cameraCapture : cameraCaptures) {
					cameraCapture->laserCaptures.selectAll();
					auto laserCaptures = cameraCapture->laserCaptures.getAllCaptures();
					for (auto laserCapture : laserCaptures) {
						laserCapture->beamCaptures.selectAll();
					}
				}
			}

			//----------
			void
				Calibrate::selectChildrenWithSolvedLines()
			{
				auto cameraCaptures = this->cameraCaptures.getSelection();
				for (auto cameraCapture : cameraCaptures) {
					auto laserCaptures = cameraCapture->laserCaptures.getAllCaptures();
					for (auto laserCapture : laserCaptures) {
						auto good = laserCapture->linesWithCommonPointSolveResult.success
							&& !laserCapture->parameters.markBad.get();
						laserCapture->setSelected(good);
					}
				}
			}

			//----------
			void
				Calibrate::deselectChildrenWithBadLines()
			{
				auto cameraCaptures = this->cameraCaptures.getAllCaptures();
				for (auto cameraCapture : cameraCaptures) {
					auto laserCaptures = cameraCapture->laserCaptures.getSelection();
					bool hasLaserCapture = false;
					for (auto laserCapture : laserCaptures) {
						auto good = laserCapture->linesWithCommonPointSolveResult.success
							&& !laserCapture->parameters.markBad.get();
						if (!good) {
							laserCapture->setSelected(false);
						}
						else {
							hasLaserCapture = true;
						}
					}
					if (!hasLaserCapture) {
						cameraCapture->setSelected(false);
					}
				}
			}
		}
	}
}
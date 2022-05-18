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
				ss << "residual: " << this->residual;
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
				Utils::serialize(json, "laserAddress", this->laserAddress);
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
				Utils::deserialize(json, "laserAddress", this->laserAddress);
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
						if (it->parameters.communications.address.get() == this->laserAddress) {
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

				widgets.push_back(make_shared<ofxCvGui::Widgets::LiveValue<string>>("Laser #" + ofToString(this->laserAddress), [this]() {
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
					panel->addButton("Select children with good lines", [this]() {
						this->selectChildrenWithGoodLines();
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
							this->calibrateBundleAdjustLasers();
						}
						RULR_CATCH_ALL_TO_ALERT;
						}, '4');
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
				this->throwIfMissingAConnection<Lasers>();
				auto lasersNode = this->getInput<Lasers>();
				auto allLasers = lasersNode->getLasersSelected();

				auto calibrationImagePoints = this->getCalibrationImagePoints();

				bool dryRun = this->parameters.capture.dryRun.enabled.get();

				Utils::ScopedProcess scopedProcess("Calibrate", false, allLasers.size());

				auto cameraCapture = make_shared<CameraCapture>();
				cameraCapture->parentSelection = &this->cameraEditSelection;

				// iterate through and send test beams
				for (const auto & laser : allLasers) {
					Utils::ScopedProcess scopedProcessLaser("Laser #" + ofToString(laser->parameters.communications.address.get()), false);

					auto laserCapture = make_shared<LaserCapture>();
					laserCapture->laserAddress = laser->parameters.communications.address.get();
					laserCapture->parentSelection = &cameraCapture->ourSelection;

					// Gather other lasers
					auto otherLasers = allLasers;
					otherLasers.erase(std::remove(otherLasers.begin(), otherLasers.end(), laser), otherLasers.end());
					
					// Set all others to laserStateForOthers
					if (this->parameters.capture.laserStateForOthers.get() == LaserState::TestPattern) {
						lasersNode->sendTestImageTo(otherLasers);
					}
					else {
						for (const auto & otherLaser : otherLasers) {
							otherLaser->parameters.deviceState.state.set(Laser::State::Standby);
							otherLaser->pushState();
						}
					}

					// Set the state for this laser
					laser->parameters.deviceState.state.set(Laser::State::Run);
					laser->pushState();

					Utils::ScopedProcess scopedProcessImagePoints("Image points", false, calibrationImagePoints.size());

					// Iterate thorugh the calibration image points
					for (const auto& calibrationImagePoint : calibrationImagePoints) {
						try {
							Utils::ScopedProcess scopedProcessImagePoint(ofToString(calibrationImagePoint));

							auto beamCapture = make_shared<BeamCapture>();
							beamCapture->projectionPoint = calibrationImagePoint;
							beamCapture->parentSelection = &laserCapture->ourSelection;

							// Originally we were offsetting when rendering
							// Now we send raw beams out without offset during calibration
							beamCapture->isOffset = false; 

							// Background capture
							for (int i = 0; i < this->parameters.capture.signalSends.get(); i++) {
								laser->parameters.deviceState.state.set(Laser::State::Standby);
								laser->pushState();
							}
							this->waitForDelay();
							if (!dryRun) {
								beamCapture->offImage.pathOnCamera = this->captureToURL();
							}

							// Positive capture image
							for (int i = 0; i < this->parameters.capture.signalSends.get(); i++) {
								laser->drawCalibrationBeam(calibrationImagePoint);
								laser->parameters.deviceState.state.set(Laser::State::Run);
								laser->pushState();
							}
							this->waitForDelay();
							if (!dryRun) {
								beamCapture->onImage.pathOnCamera = this->captureToURL();
							}

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

					if (laserCapture->beamCaptures.size() > 0) {
						cameraCapture->laserCaptures.add(laserCapture);
					}
				}

				// Set all to laserStateForOthers
				if (this->parameters.capture.laserStateForOthers.get() == LaserState::TestPattern) {
					lasersNode->sendTestImageTo(allLasers);
				}
				else {
					for (const auto& laser : allLasers) {
						for (int i = 0; i < this->parameters.capture.signalSends.get(); i++) {
							laser->parameters.deviceState.state.set(Laser::State::Standby);
							laser->pushState();
						}
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
								&& laserCapture->laserAddress == laser->parameters.communications.address.get()) {
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
							if (it->parameters.communications.address.get() == laserCapture->laserAddress) {
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
										<< "::" << laserCapture->laserAddress
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
			void
				Calibrate::configureSolverSettings(ofxCeres::SolverSettings& solverSettings) const
			{
				solverSettings.printReport = this->parameters.solver.printOutput;
				solverSettings.options.minimizer_progress_to_stdout = this->parameters.solver.printOutput;
				solverSettings.options.max_num_iterations = this->parameters.solver.maxIterations;
				solverSettings.options.num_threads = this->parameters.solver.threads;
				solverSettings.options.function_tolerance = this->parameters.solver.functionTolerance.get();
				solverSettings.options.parameter_tolerance = this->parameters.solver.parameterTolerance.get();
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
				Calibrate::selectChildrenWithGoodLines()
			{
				auto cameraCaptures = this->cameraCaptures.getSelection();
				for (auto cameraCapture : cameraCaptures) {
					auto laserCaptures = cameraCapture->laserCaptures.getAllCaptures();
					for (auto laserCapture : laserCaptures) {
						laserCapture->setSelected(laserCapture->linesWithCommonPointSolveResult.success);
					}
				}
			}
		}
	}
}
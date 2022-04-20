#include "pch_Plugin_Scrap.h"
#include "Calibrate.h"
#include "Lasers.h"

namespace ofxRulr {
	namespace Nodes {
		namespace AnotherMoon {
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
				ss << "s: " << this->line.s << ", t: "<< this->line.t;
				return ss.str();
			}

			//----------
			void
				Calibrate::BeamCapture::serialize(nlohmann::json& json)
			{
				Utils::serialize(json, "imagePoint", this->imagePoint);
				Utils::serialize(json, "urlOnImage", this->urlOnImage);
				Utils::serialize(json, "urlOffImage", this->urlOffImage);

				this->line.serialize(json["line"]);
			}

			//----------
			void
				Calibrate::BeamCapture::deserialize(const nlohmann::json& json)
			{
				Utils::deserialize(json, "imagePoint", this->imagePoint);
				Utils::deserialize(json, "urlOnImage", this->urlOnImage);
				Utils::deserialize(json, "urlOffImage", this->urlOffImage);

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
				widgets.push_back(make_shared<ofxCvGui::Widgets::Toggle>(">>"
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
					}));


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

			}

			//----------
			void
				Calibrate::LaserCapture::deserialize(const nlohmann::json& json)
			{
				if (json.contains("beamCaptures")) {
					this->beamCaptures.deserialize(json["beamCaptures"]);
				}
				Utils::deserialize(json, "laserAddress", this->laserAddress);

				auto beamCaptures = this->beamCaptures.getAllCaptures();
				for (auto beamCapture : beamCaptures) {
					beamCapture->parentSelection = &this->ourSelection;
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
				widgets.push_back(make_shared<ofxCvGui::Widgets::Toggle>(">>"
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
					}));


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
			}

			//----------
			void
				Calibrate::CameraCapture::deserialize(const nlohmann::json& json)
			{
				if (json.contains("laserCaptures")) {
					this->laserCaptures.deserialize(json["laserCaptures"]);
				}
				
				auto laserCaptures = this->laserCaptures.getAllCaptures();
				for (auto capture : laserCaptures) {
					capture->parentSelection = &this->ourSelection;
				}
			}

			//----------
			ofxCvGui::ElementPtr
				Calibrate::CameraCapture::getDataDisplay()
			{
				auto element = ofxCvGui::makeElement();

				vector<ofxCvGui::ElementPtr> widgets;

				widgets.push_back(make_shared<ofxCvGui::Widgets::LiveValue<string>>("", [this]() {
					return this->getDisplayString();
					}));
				widgets.push_back(make_shared<ofxCvGui::Widgets::Toggle>(">>"
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
					}));


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

				RULR_NODE_INSPECTOR_LISTENER;
				RULR_NODE_SERIALIZATION_LISTENERS;

				this->manageParameters(this->parameters);

				{
					// Make the panel
					auto panel = ofxCvGui::Panels::makeWidgets();
					this->cameraCaptures.populateWidgets(panel);
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
				Calibrate::populateInspector(ofxCvGui::InspectArguments& inspectArgs)
			{
				auto inspector = inspectArgs.inspector;

				inspector->addButton("Capture", [this]() {
					try {
						this->capture();
					}
					RULR_CATCH_ALL_TO_WARNING;
					}, ' ');
				inspector->addButton("Process", [this]() {
					try {
						this->process();
					}
					RULR_CATCH_ALL_TO_ALERT;
					}, 'p');
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
				Calibrate::capture()
			{
				this->throwIfMissingAConnection<Lasers>();
				auto lasersNode = this->getInput<Lasers>();
				auto allLasers = lasersNode->getSelectedLasers();

				auto calibrationImagePoints = this->getCalibrationImagePoints();

				bool dryRun = this->parameters.capture.dryRun.enabled.get();

				Utils::ScopedProcess scopedProcess("Calibrate", false, allLasers.size());

				auto cameraCapture = make_shared<CameraCapture>();
				cameraCapture->parentSelection = &this->cameraEditSelection;

				// iterate through and send test beams
				for (const auto & laser : allLasers) {
					Utils::ScopedProcess scopedProcessLaser("Laser #" + ofToString(laser->parameters.settings.address.get()), false);

					auto laserCapture = make_shared<LaserCapture>();
					laserCapture->laserAddress = laser->parameters.settings.address.get();
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
							otherLaser->standby();
						}
					}

					// Set the state for this laser
					for (int i = 0; i < this->parameters.capture.signalSends.get(); i++) {
						laser->run();
					}

					Utils::ScopedProcess scopedProcessImagePoints("Image points", false, calibrationImagePoints.size());

					// Iterate thorugh the calibration image points
					for (const auto& calibrationImagePoint : calibrationImagePoints) {
						try {
							Utils::ScopedProcess scopedProcessImagePoint(ofToString(calibrationImagePoint));

							auto beamCapture = make_shared<BeamCapture>();
							beamCapture->imagePoint = calibrationImagePoint;
							beamCapture->parentSelection = &laserCapture->ourSelection;

							// Background capture
							for (int i = 0; i < this->parameters.capture.signalSends.get(); i++) {
								laser->standby();
							}
							this->waitForDelay();
							if (!dryRun) {
								beamCapture->urlOffImage = this->captureToURL();
							}

							// Positive capture image
							for (int i = 0; i < this->parameters.capture.signalSends.get(); i++) {
								laser->drawCircle(calibrationImagePoint, 0.0f);
								laser->run();
							}
							this->waitForDelay();
							if (!dryRun) {
								beamCapture->urlOnImage = this->captureToURL();
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
							laser->standby();
						}
					}
				}

				if (!dryRun) {
					this->cameraCaptures.add(cameraCapture);
				}
			}

			//----------
			void
				Calibrate::process()
			{
				Utils::ScopedProcess scopedProcess("Process");

				this->throwIfMissingAConnection<Lasers>();
				auto lasersNode = this->getInput<Lasers>();
				auto selectedLasers = lasersNode->getSelectedLasers();

				auto cameraCaptures = this->cameraCaptures.getSelection();

				{
					Utils::ScopedProcess scopedProcessCameras("Cameras", false, cameraCaptures.size());

					for (auto cameraCapture : cameraCaptures) {
						auto laserCaptures = cameraCapture->laserCaptures.getSelection();
						Utils::ScopedProcess scopedProcessCameras("Camera : " + cameraCapture->getDateString() + " " + cameraCapture->getTimeString(), false, laserCaptures.size());

						for (auto laserCapture : laserCaptures) {
							auto beamCaptures = laserCapture->beamCaptures.getSelection();
							Utils::ScopedProcess scopedProcessLasers("Laser #" + ofToString(laserCapture->laserAddress), false, beamCaptures.size());

							vector<Solvers::LinesFromPoint::CameraImagePoints> images;

							cv::Mat preview;
							
							// Gather image points per beam in laser
							for (auto beamCapture : beamCaptures) {
								Utils::ScopedProcess scopedProcessBeam("Beam : " + ofToString(beamCapture->imagePoint), false, beamCaptures.size());

								// Get the on and off images
								auto onImage = this->fetchImage(beamCapture->urlOnImage);
								auto offImage = this->fetchImage(beamCapture->urlOffImage);

								// Get the positive difference
								cv::Mat difference;
								cv::subtract(onImage, offImage, difference);

								// Allocate preview if needs be
								if (preview.empty()) {
									preview = cv::Mat::zeros(difference.size(), difference.type());
								}

								// Get the camera image points for this image
								images.push_back(Solvers::LinesFromPoint::getCameraImagePoints(difference
									, this->parameters.processing.normalizePercentile.get()
									, this->parameters.processing.differenceThreshold.get()
									, preview));
							}

							// Solve lines for the whole laser
							auto solverSettings = Solvers::LinesFromPoint::defaultSolverSettings();
							this->configureSolverSettings(solverSettings);
							auto result = Solvers::LinesFromPoint::solve(images
								, this->parameters.processing.distanceThreshold.get()
								, this->parameters.processing.minMeanPixelValueOnLine.get()
								, solverSettings);

							// Pull data back from solve
							laserCapture->imagePointInCamera = result.solution.point;
							for (int i = 0; i < beamCaptures.size(); i++) {
								beamCaptures[i]->line = result.solution.lines[i];
								if (!result.solution.linesValid[i]) {
									beamCaptures[i]->setSelected(false);
								}
								else {
									beamCaptures[i]->line.drawOnImage(preview);
								}
							}

							laserCapture->preview = preview;
						}
					}
				}

				scopedProcess.end();
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
				ofHttpRequest request;
				{
					request.method = ofHttpRequest::GET;
					request.url = this->getBaseCameraURL() + "event/polling";
				}

				auto response = this->urlLoader.handleRequest(request);

				if (response.status != 200) {
					throw(ofxRulr::Exception("Couldn't poll camera : " + (string)response.data));
				}

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
				Calibrate::fetchImage(const string& cameraPath) const
			{
				auto flags = cv::IMREAD_GRAYSCALE;

				switch (this->parameters.processing.imageFileSource.get()) {
					case ImageFileSource::Local:
					{
						auto splitPath = ofSplitString(cameraPath, "/");
						auto filename = splitPath.back();
						auto localPath = filesystem::path(this->parameters.processing.localPath.get());
						if (localPath.empty()) {
							throw(ofxRulr::Exception("Local path is empty"));
						}
						if (!filesystem::is_directory(localPath)) {
							localPath = localPath.parent_path();
						}
						auto fullFilePath = localPath / filename;

						auto image = cv::imread(fullFilePath.string(), flags);
						if (image.empty()) {
							throw(Exception("Failed to load image at " + fullFilePath.string()));
						}
						return image;
					}
					case ImageFileSource::Camera:
					{
						auto response = ofLoadURL("http://" + this->parameters.capture.remoteCamera.hostname.get() + ":8080" + cameraPath);
						if (response.status == 200) {
							auto data = response.data.getData();
							vector<unsigned char> buffer(data, data + response.data.size());
							return cv::imdecode(buffer, flags);
						}
						else {
							throw(Exception("Failed to load image from camera : " + (string) response.data));
						}
					}
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
			}
		}
	}
}
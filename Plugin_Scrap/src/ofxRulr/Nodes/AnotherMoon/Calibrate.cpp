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
				ss << this->imagePoint;
				return ss.str();
			}

			//----------
			void
				Calibrate::BeamCapture::serialize(nlohmann::json& json)
			{
				Utils::serialize(json, "imagePoint", this->imagePoint);
				Utils::serialize(json, "urlOnImage", this->urlOnImage);
				Utils::serialize(json, "urlOffImage", this->urlOffImage);
			}

			//----------
			void
				Calibrate::BeamCapture::deserialize(const nlohmann::json& json)
			{
				Utils::deserialize(json, "imagePoint", this->imagePoint);
				Utils::deserialize(json, "urlOnImage", this->urlOnImage);
				Utils::deserialize(json, "urlOffImage", this->urlOffImage);
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

				// iterate through and send test beams
				for (const auto & laser : allLasers) {
					Utils::ScopedProcess scopedProcessLaser("Laser #" + ofToString(laser->parameters.settings.address.get()), false);

					auto laserCapture = make_shared<LaserCapture>();
					laserCapture->laserAddress = laser->parameters.settings.address.get();

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
		}
	}
}
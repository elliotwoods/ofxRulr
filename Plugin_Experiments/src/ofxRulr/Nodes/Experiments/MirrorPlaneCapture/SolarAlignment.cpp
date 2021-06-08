#include "pch_Plugin_Experiments.h"

float getMarkerReprojectionError2(vector<cv::Point2f> imagePoints, cv::Mat cameraMatrix, cv::Mat distortionCoefficients) {
	cv::Mat translation, rotationVector;

	vector<cv::Point3f> dummyObjectPoints;
	{
		dummyObjectPoints.emplace_back(0, 0, 0);
		dummyObjectPoints.emplace_back(1, 0, 0);
		dummyObjectPoints.emplace_back(1, 1, 0);
		dummyObjectPoints.emplace_back(0, 1, 0);
	}

	cv::solvePnP(dummyObjectPoints
		, imagePoints
		, cameraMatrix
		, distortionCoefficients
		, rotationVector
		, translation);

	vector<cv::Point2f> reprojectedPoints;
	cv::projectPoints(dummyObjectPoints
		, rotationVector
		, translation
		, cameraMatrix
		, distortionCoefficients
		, reprojectedPoints);

	//get RMS
	float accumulate = 0.0f;
	for (int i = 0; i < imagePoints.size(); i++) {
		auto delta = reprojectedPoints[i] - imagePoints[i];
		accumulate += delta.dot(delta);
	}
	auto MS = accumulate / imagePoints.size();
	return sqrt(MS);
}

//function to test reprojection error for a set of points given a translation, rotation
float getReprojectionError2 (shared_ptr<ofxRulr::Nodes::Item::Camera> camera, const vector<cv::Point3f> & objectPoints
	, const vector<cv::Point2f> & imagePoints
	, const cv::Mat & rotationVector
	, const cv::Mat & translation) {

	float error = 0.0f;
	vector<cv::Point2f> reprojectedPoints;
	cv::projectPoints(objectPoints
		, rotationVector
		, translation
		, camera->getCameraMatrix()
		, camera->getDistortionCoefficients()
		, reprojectedPoints);

	//Take square mean
	for (size_t i = 0; i < imagePoints.size(); i++) {
		auto delta = imagePoints[i] - reprojectedPoints[i];
		error += delta.x * delta.x + delta.y * delta.y;
	}
	//RMS
	error /= (float)imagePoints.size();
	error = sqrt(error);

	return error;
};

namespace ofxRulr {
	namespace Nodes {
		namespace Experiments {
			namespace MirrorPlaneCapture {
#pragma mark Captures
				//----------
				SolarAlignment::SolarAlignment() {
					RULR_NODE_INIT_LISTENER;
				}

				//----------
				std::string SolarAlignment::getTypeName() const {
					return "HALO::SolarAlignment";
				}

				//----------
				void SolarAlignment::init() {
					RULR_NODE_INSPECTOR_LISTENER;
					RULR_NODE_SERIALIZATION_LISTENERS;
					RULR_NODE_DRAW_WORLD_LISTENER;

					{
						auto panel = ofxCvGui::Panels::makeImage(this->preview);
						panel->onDrawImage += [this](ofxCvGui::DrawImageArguments & args) {
							auto captures = this->captures.getSelection();
							for (auto capture : captures) {
								capture->draw();
							}
						};
						this->panel = panel;
					}

					this->addInput<Item::Camera>();
					this->addInput<ArUco::MarkerMap>();

					this->manageParameters(this->parameters);
				}

				//----------
				void SolarAlignment::populateInspector(ofxCvGui::InspectArguments & inspectArgs) {
					auto inspector = inspectArgs.inspector;
					this->captures.populateWidgets(inspector);
					inspector->addButton("Add capture", [this]() {
						try {
							Utils::ScopedProcess scopedProcess("Add capture");
							this->addCapture();
							scopedProcess.end();
						}
						RULR_CATCH_ALL_TO_ALERT;
					}, ' ');
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
				void SolarAlignment::serialize(nlohmann::json & json) {
					this->captures.serialize(json["captures"]);
				}

				//----------
				void SolarAlignment::deserialize(const nlohmann::json & json) {
					this->captures.deserialize(json["captures"]);
				}

				//----------
				ofxCvGui::PanelPtr SolarAlignment::getPanel() {
					return this->panel;
				}

				//----------
				void SolarAlignment::drawWorldStage() {
					auto captures = this->captures.getSelection();
					for (auto capture : captures) {
						capture->drawWorld();
					}
				}

				//----------
				void SolarAlignment::addCapture() {
					this->throwIfMissingAnyConnection();
					auto camera = this->getInput<Item::Camera>();

					auto capture = make_shared<Capture>();
					
					//navigate the camera 
					{
						Utils::ScopedProcess scopedProcess("Marker map navigation image", false);
						auto markerMap = this->getInput<ArUco::MarkerMap>();
						markerMap->throwIfMissingAConnection<ArUco::Detector>();
						auto detector = markerMap->getInput<ArUco::Detector>();

						auto frame = camera->getFreshFrame();
						cv::Mat image = ofxCv::toCv(frame->getPixels());

						auto markers = detector->findMarkers(image, false);
						if (markers.size() < this->parameters.cameraNavigation.minimumMarkers) {
							throw(ofxRulr::Exception("Found " + ofToString(markers.size()) + " real markers. Need " + ofToString(this->parameters.cameraNavigation.minimumMarkers) + " for camera navigation"));
						}

						//build up dataset
						vector<cv::Point3f> worldPoints;
						vector<cv::Point2f> imagePoints;
						{
							int countGoodMarkers = 0;
							for (const auto & marker : markers) {
								try {
									//test reprojection error
									auto markerReprojectionError = getMarkerReprojectionError2((vector<cv::Point2f> &) marker
										, camera->getCameraMatrix()
										, camera->getDistortionCoefficients());

									if (markerReprojectionError <= this->parameters.cameraNavigation.maxMarkerError) {
										countGoodMarkers++;
										auto marker3D = markerMap->getMarkerMap()->getMarker3DInfo(marker.id);
										for (int i = 0; i < 4; i++) {
											worldPoints.push_back(marker3D.points[i]);
											imagePoints.push_back(marker[i]);
										}
									}
								}
								RULR_CATCH_ALL_TO_ERROR; //in the case that the marker did not exist
							}

							if (countGoodMarkers < this->parameters.cameraNavigation.minimumMarkers) {
								throw(ofxRulr::Exception("Found " + ofToString(countGoodMarkers) + " good markers in map. Need " + ofToString(this->parameters.cameraNavigation.minimumMarkers) + " for camera navigation"));
							}
						}

						cv::Mat rotationVector, translation;
						camera->getExtrinsics(rotationVector, translation, true);

						//Solve camera extrinsics
						{
							//first pass EPNP
							cv::solvePnPRansac(worldPoints
								, imagePoints
								, camera->getCameraMatrix()
								, camera->getDistortionCoefficients()
								, rotationVector
								, translation
								, true
								, 100
								, 5.0f
								, 0.99
								, cv::noArray()
								, cv::SOLVEPNP_EPNP);

							//second pass ITERATIVE
							cv::solvePnPRansac(worldPoints
								, imagePoints
								, camera->getCameraMatrix()
								, camera->getDistortionCoefficients()
								, rotationVector
								, translation
								, true
								, 100
								, 5.0f
								, 0.99
								, cv::noArray()
								, cv::SOLVEPNP_ITERATIVE);
						}

						camera->setExtrinsics(rotationVector, translation, true);
						
						capture->cameraNavigation.imagePoints = imagePoints;
						capture->cameraNavigation.worldPoints = worldPoints;
						capture->cameraNavigation.reprojectionError = getReprojectionError2(camera
							, worldPoints
							, imagePoints
							, rotationVector
							, translation);
						capture->cameraNavigation.cameraPosition = camera->getPosition();
					}

					//get the solar vector
					{
						Utils::ScopedProcess scopedProcess("Solar centroid image", false);
						auto frame = camera->getFreshFrame();
						cv::Mat image = ofxCv::toCv(frame->getPixels());

						if (image.channels() == 3) {
							cv::cvtColor(image, image, cv::COLOR_RGB2GRAY);
						}

						cv::Mat binary;

						cv::threshold(image
							, binary
							, this->parameters.solarCentroid.threshold
							, 255
							, cv::THRESH_BINARY);

						vector<vector<cv::Point2i>> contours;
						vector<cv::Point2f> centroids;
						vector<cv::Rect> boundingRects;
						vector<cv::Moments> moments;

						cv::findContours(binary
							, contours
							, cv::RETR_EXTERNAL
							, cv::CHAIN_APPROX_NONE);

						if (contours.size() < 1) {
							throw(ofxRulr::Exception("Found 0 contours. Need 1"));
						}

						vector<cv::Point2i> bestContour;
						float bestArea = 0;
						for (const auto & contour : contours) {
							auto bounds = cv::boundingRect(contour);
							auto area = bounds.area();
							if (area > bestArea) {
								bestContour = contour;
								bestArea = area;
							}
						}

						cv::Rect2i dilatedRect = cv::boundingRect(bestContour);
						{
							dilatedRect.x -= 5;
							dilatedRect.y -= 5;
							dilatedRect.width += 5;
							dilatedRect.height += 5;
						}

						auto moment = cv::moments(image(dilatedRect));
						capture->centroid = glm::vec2(moment.m10 / moment.m00 + dilatedRect.x
							, moment.m01 / moment.m00 + dilatedRect.y);

						auto cameraView = camera->getViewInWorldSpace();

						capture->ray = cameraView.castPixel(capture->centroid, true);

						cv::Mat preview;
						image.copyTo(preview, binary);
						ofxCv::copy(preview, this->preview);
						this->preview.update();
					}

					//get the timestamp of the file
					{
						auto result = ofSystemTextBoxDialog("Epoch timestamp for capture");
						if (result.empty()) {
							return;
						}

						time_t epochTime = ofToInt(result);
						capture->timestamp.set(chrono::system_clock::from_time_t(epochTime));
						capture->rebuildDateStrings();
					}

					//get the solar tracking for this time
					{
						auto result = ofSystemTextBoxDialog("Azimuth, Altitude from sunPosition.py");
						if (result.empty()) {
							return;
						}

						stringstream ss(result);
						ss >> capture->azimuthAltitude;
					}

					//calculate the solar vector in global frame
					{
						auto azRotate = ofMatrix4x4::newRotationMatrix(capture->azimuthAltitude.x, 0, 1, 0);
						auto elRotate = ofMatrix4x4::newRotationMatrix(capture->azimuthAltitude.y, +1, 0, 0);
						capture->solarVectorFromPySolar = Utils::applyTransform(azRotate * elRotate, glm::vec3(0, 0, -1));
					}

					this->captures.add(capture);
				}

				//----------
				void SolarAlignment::calibrate() {

				}

#pragma mark Capture
				//----------
				SolarAlignment::Capture::Capture() {
					RULR_SERIALIZE_LISTENERS;
				}

				//----------
				std::string SolarAlignment::Capture::getDisplayString() const {
					stringstream ss;
					ss << "Solar vector : " << this->ray.t << endl;
					ss << "Azi/Alt : " << this->azimuthAltitude << endl;
					return ss.str();
				}

				//----------
				void SolarAlignment::Capture::drawWorld() {
					this->ray.color = this->color;
					this->ray.draw();

					ofPushStyle();
					{
						ofPushMatrix();
						{
							ofSetColor(this->color);
							ofRotateDeg(this->azimuthAltitude.x, 0, +1, 0);
							ofRotateDeg(this->azimuthAltitude.y, +1, 0, 0);
							ofDrawLine(glm::vec3(), glm::vec3(0, 0, -10));
						}
						ofPopMatrix();

						ofPushMatrix();
						{
							ofTranslate(0.1, 0.0, 0.0);
							ofDrawLine(glm::vec3(), this->solarVectorFromPySolar);
						}
						ofPopMatrix();
					}
					ofPopStyle();
				}

				//----------
				void SolarAlignment::Capture::serialize(nlohmann::json & json) {
					json["ray_s"] << this->ray.s;
					json["ray_t"] << this->ray.t;
					json["centroid"] << this->centroid;
					json["azimuthAltitude"] << this->azimuthAltitude;

					{
						auto & jsonCameraNavigation = json["cameraNavigation"];
						jsonCameraNavigation["imagePoints"] << ofxCv::toOf(this->cameraNavigation.imagePoints);
						jsonCameraNavigation["worldPoints"] << ofxCv::toOf(this->cameraNavigation.worldPoints);
						jsonCameraNavigation["cameraPosition"] << this->cameraNavigation.cameraPosition;
						Utils::serialize(jsonCameraNavigation, this->cameraNavigation.reprojectionError);
					}
				}

				//----------
				void SolarAlignment::Capture::deserialize(const nlohmann::json & json) {
					json["ray_s"] >> this->ray.s;
					json["ray_t"] >> this->ray.t;
					this->ray.defined = true;
					json["centroid"] >> this->centroid;
					json["azimuthAltitude"] >> this->azimuthAltitude;

					{
						const auto & jsonCameraNavigation = json["cameraNavigation"];
						jsonCameraNavigation["imagePoints"] >> ofxCv::toOf(this->cameraNavigation.imagePoints);
						jsonCameraNavigation["worldPoints"] >> ofxCv::toOf(this->cameraNavigation.worldPoints);
						jsonCameraNavigation["cameraPosition"] >> this->cameraNavigation.cameraPosition;
						Utils::deserialize(jsonCameraNavigation, this->cameraNavigation.reprojectionError);
					}
				}

				//----------
				void SolarAlignment::Capture::draw() {
					ofPushStyle();
					{
						ofSetColor(this->color);
						ofPushMatrix();
						{
							ofTranslate(this->centroid);
							ofDrawLine(-10, 0, 10, 0);
							ofDrawLine(0, -10, 0, 10);
						}
						ofPopMatrix();
					}
					ofPopStyle();
				}
			}
		}
	}
}
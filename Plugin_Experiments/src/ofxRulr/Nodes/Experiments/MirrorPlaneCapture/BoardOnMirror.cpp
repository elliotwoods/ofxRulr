#include "pch_Plugin_Experiments.h"
#include "planesToPoint.h"

template<typename T>
T mean(const vector<T> & points) {
	T accumulator(0);

	for (const auto & point : points) {
		accumulator += point;
	}
	return accumulator / size(points);
}

#include "BoardOnMirror.h"

float getMarkerReprojectionError(vector<cv::Point2f> imagePoints, cv::Mat cameraMatrix, cv::Mat distortionCoefficients) {
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


namespace ofxRulr {
	namespace Nodes {
		namespace Experiments {
			namespace MirrorPlaneCapture {
				//----------
				BoardOnMirror::BoardOnMirror() {
					RULR_NODE_INIT_LISTENER;
				}

				//----------
				string BoardOnMirror::getTypeName() const {
					return "Experiments::MirrorPlaneCapture::BoardOnMirror";
				}

				//----------
				void BoardOnMirror::init() {
					RULR_NODE_DRAW_WORLD_LISTENER;
					RULR_NODE_INSPECTOR_LISTENER;
					RULR_NODE_UPDATE_LISTENER;
					RULR_NODE_SERIALIZATION_LISTENERS;

					this->addInput<Item::Camera>();
					this->addInput<Item::AbstractBoard>();
					this->addInput<ArUco::Detector>();
					this->addInput<ArUco::MarkerMap>();
					this->addInput<Heliostats>();

					this->panel = make_shared<ofxCvGui::Panels::Widgets>();
					this->captures.populateWidgets(this->panel);

					this->manageParameters(this->parameters);
				}

				//----------
				void BoardOnMirror::update() {

				}

				//----------
				void BoardOnMirror::addCapture() {
					this->throwIfMissingAConnection<Item::Camera>();

					auto camera = this->getInput<Item::Camera>();
					auto grabber = camera->getGrabber();
					if (!grabber) {
						throw(ofxRulr::Exception("Camera grabber not available"));
					}

					auto frame = grabber->getFreshFrame();
					auto image = ofxCv::toCv(frame->getPixels());
					this->addImage(image);
				}

				//----------
				void BoardOnMirror::calibrate() {

				}

				//----------
				ofxCvGui::PanelPtr BoardOnMirror::getPanel() {
					return this->panel;
				}

				//----------
				void BoardOnMirror::populateInspector(ofxCvGui::InspectArguments & inspectArgs) {
					auto inspector = inspectArgs.inspector;
					inspector->addButton("Add capture", [this]() {
						try {
							Utils::ScopedProcess scopedProcess("Add capture");
							this->addCapture();
							scopedProcess.end();
						}
						RULR_CATCH_ALL_TO_ALERT;
					}, ' ')->setHeight(100.0f);

					inspector->addButton("Add folder of images...", [this]() {
						auto result = ofSystemLoadDialog("Folder of calibration images", true);
						if (result.bSuccess) {
							this->addFolderOfImages(std::filesystem::path(result.getPath()));
						}
					});

					inspector->addButton("Calibrate", [this]() {
						try {
							this->calibrate();
						}
						RULR_CATCH_ALL_TO_ALERT;
					}, OF_KEY_RETURN)->setHeight(100.0f);

					inspector->addButton("Mean plane to clipboard", [this]() {
						try {
							this->copyMeanPlaneToClipboard();
						}
						RULR_CATCH_ALL_TO_ALERT;
					}, OF_KEY_RETURN)->setHeight(100.0f);

					inspector->addButton("Project heliostats from planes", [this]() {
						try {
							this->projectToHeliostats();
						}
						RULR_CATCH_ALL_TO_ALERT;
					});
				}

				//----------
				void BoardOnMirror::drawWorldStage() {
					auto captures = this->captures.getSelection();
					for (auto capture : captures) {
						capture->drawWorld();
					}
				}

				//----------
				void BoardOnMirror::addImage(const cv::Mat & image, const string & name) {
					this->throwIfMissingAConnection<Item::Camera>();
					this->throwIfMissingAConnection<Item::AbstractBoard>();

					auto camera = this->getInput<Item::Camera>();
					auto board = this->getInput<Item::AbstractBoard>();

					auto capture = make_shared<Capture>();

					capture->name = name;

					//function to test reprojection error for a set of points given a translation, rotation
					auto getReprojectionError = [&camera](const vector<cv::Point3f> & objectPoints
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

					//resolve pose of camera using the marker map
					capture->cameraNavigation.enabled = this->parameters.cameraNavigation.enabled;
					if (this->parameters.cameraNavigation.enabled) {
						this->throwIfMissingAConnection<ArUco::MarkerMap>();
						this->throwIfMissingAConnection<ArUco::Detector>();

						auto detector = this->getInput<ArUco::Detector>();
						auto markerMap = this->getInput<ArUco::MarkerMap>()->getMarkerMap();

						auto markers = detector->findMarkers(image);
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
									auto markerReprojectionError = getMarkerReprojectionError((vector<cv::Point2f> &) marker
										, camera->getCameraMatrix()
										, camera->getDistortionCoefficients());

									if (markerReprojectionError <= this->parameters.cameraNavigation.maxMarkerError) {
										countGoodMarkers++;
										auto marker3D = markerMap->getMarker3DInfo(marker.id);
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
						capture->cameraNavigation.reprojectionError = getReprojectionError(worldPoints
							, imagePoints
							, rotationVector
							, translation);
						capture->cameraNavigation.cameraPosition = camera->getPosition();
					}

					auto cameraTransform = camera->getTransform();

					//--
					//find board plane function
					//
					//
					auto findBoardPlane = [this, camera, board, &getReprojectionError, &cameraTransform](const cv::Mat & image, Capture::Plane & plane) {

						//get board image and object space points
						if (!board->findBoard(image
							, plane.imagePoints
							, plane.objectPoints
							, this->parameters.capture.findBoardMode
							, camera->getCameraMatrix()
							, camera->getDistortionCoefficients())) {
							throw(Exception("Failed to find board"));
						}

						//check we found enough corners
						if (plane.objectPoints.size() < this->parameters.capture.minimumCorners) {
							throw(Exception("Not enough corners found (" + ofToString(plane.objectPoints.size()) + " found. Needs " + ofToString(this->parameters.capture.minimumCorners) + ")"));
						}

						cv::Mat rotation, translation;
						{
							float acceptableReprojectionError = ofClamp(camera->getWidth() / 1000, 1, 10);

							//second pass ITERATIVE
							cv::solvePnP(plane.objectPoints
								, plane.imagePoints
								, camera->getCameraMatrix()
								, camera->getDistortionCoefficients()
								, rotation
								, translation
								, false
								, cv::SOLVEPNP_ITERATIVE);
						}

						//record reprojection error
						plane.reprojectionError = getReprojectionError(plane.objectPoints
							, plane.imagePoints
							, rotation
							, translation);

						//build up world space points
						plane.boardTransform = ofxCv::makeMatrix(rotation, translation) * cameraTransform;

						for (const auto & objectPoint : plane.objectPoints) {
							plane.worldPoints.push_back(Utils::applyTransform(plane.boardTransform, ofxCv::toOf(objectPoint)));
						}

						if (!plane.plane.fitToPoints(plane.worldPoints, plane.planeFitResidual)) {
							throw(Exception("Failed to fit board plane"));
						}

						//take mean of object space points
						plane.meanObjectPoint = mean(ofxCv::toOf(plane.objectPoints));
						plane.meanWorldPoint = mean(plane.worldPoints);
						plane.plane.setCenter(plane.meanWorldPoint);

						//this seems to be the data we actually want. And the normal should be consistent
						plane.plane.setNormal(-(glm::vec3) ((ofMatrix4x4)plane.boardTransform).getRowAsVec3f(2));
					};
					//
					//--


					//find board plane
					findBoardPlane(image, capture->mirrorPlane);


					//log to server
					if (this->parameters.logToServer.enabled) {
						try {
							Utils::ScopedProcess scopedProcessLogToServer("Log to server");
							nlohmann::json requestJson;

							{
								auto & data = requestJson["data"];

								auto & jsonPlane = data["plane"];

								jsonPlane["ABCD"] << capture->mirrorPlane.plane.getABCD();
								jsonPlane["center"] << capture->mirrorPlane.plane.getCenter();
								jsonPlane["normal"] << capture->mirrorPlane.plane.getNormal();
							}

							ofHttpRequest request(this->parameters.logToServer.address.get(), "log");
							request.method = ofHttpRequest::Method::POST;
							request.body = requestJson.dump(4);
							request.contentType = "application/json";

							ofURLFileLoader urlLoader;
							auto response = urlLoader.handleRequest(request);
							cout << response.data;
							scopedProcessLogToServer.end();
						}
						RULR_CATCH_ALL_TO_ALERT;
					}

					this->captures.add(capture);
				}

				//----------
				void BoardOnMirror::addFolderOfImages(const std::filesystem::path & path) {
					//get list of files
					vector<string> filePaths;
					for (std::filesystem::directory_iterator it(path)
						; it != std::filesystem::directory_iterator()
						; ++it) {
						filePaths.push_back(it->path().string());
					}

					Utils::ScopedProcess scopedProcess("Adding folder images", true, filePaths.size());

					//for each file in path
					for (auto filePath : filePaths) {
						try {
							Utils::ScopedProcess fileScopedProcess(filePath, false);

							//load image
							auto image = cv::imread(filePath);

							//check it loaded
							if (image.empty()) {
								continue;
							}
							else {
								//add it to the captures
								this->addImage(image, ofFilePath::getBaseName(filePath));
							}
						}
						RULR_CATCH_ALL_TO({
							RULR_WARNING << e.what();
						});
					}

					scopedProcess.end();
				}

				//----------
				void BoardOnMirror::copyMeanPlaneToClipboard() const {
					auto captures = this->captures.getSelection();
					if (captures.empty()) {
						throw(Exception("No captures available"));
					}

					glm::vec3 meanCenter, meanNormal;
					for (auto capture : captures) {
						meanCenter += capture->mirrorPlane.plane.getCenter();
						meanNormal += capture->mirrorPlane.plane.getNormal();
					}
					meanCenter /= captures.size();
					meanNormal /= captures.size();
					meanNormal = glm::normalize(meanNormal);

					ofxRay::Plane meanPlane;
					meanPlane.setCenter(meanCenter);
					meanPlane.setNormal(meanNormal);

					stringstream ss;
					ss << meanPlane.getABCD();
					ofxClipboard::copy(ss.str());
				}

				//----------
				void BoardOnMirror::projectToHeliostats() {
					this->throwIfMissingAConnection<Heliostats>();
					auto heliostatsNode = this->getInput<Heliostats>();

					//get our selected captures
					auto captures = this->captures.getSelection();

					//existing heliostats
					auto existingHeliostats = heliostatsNode->getHeliostats();

					set<shared_ptr<Heliostats::Heliostat>> heliostatsToAdd;

					map<shared_ptr<Heliostats::Heliostat>, vector<shared_ptr<Capture>>> capturesToMove;

					for (auto capture : captures) {
						auto backPosition = capture->mirrorPlane.meanWorldPoint
							- capture->mirrorPlane.plane.getNormal()
							* this->parameters.heliostatProjection.planeToA2.get();

						//check if matching heliostat already exists
						shared_ptr<Heliostats::Heliostat> heliostat;
						for (auto heliostatOther : existingHeliostats) {
							if (glm::length(heliostatOther->position.get() - backPosition) < this->parameters.heliostatProjection.distanceThreshold) {
								heliostat = heliostatOther;
								break;
							}
						}

						//if it doesn't already exist, check if it's one we are already trying to add
						if (!heliostat) {
							for (auto heliostatOther : heliostatsToAdd) {
								if (glm::length(heliostatOther->position.get() - backPosition) < this->parameters.heliostatProjection.distanceThreshold) {
									heliostat = heliostatOther;
									break;
								}
							}
						}

						//if it doesn't already exist, create it
						if (!heliostat) {
							heliostat = make_shared<Heliostats::Heliostat>();
							heliostat->position = backPosition;


							static int index = 0;
							heliostat->name = "H-auto" + ofToString(index++);

							heliostatsToAdd.insert(heliostat);
						}

						auto dictIt = capturesToMove.find(heliostat);
						if (dictIt == capturesToMove.end()) {
							capturesToMove.emplace(heliostat, vector<shared_ptr<Capture>>());
							dictIt = capturesToMove.find(heliostat);
						}

						dictIt->second.push_back(capture);
					}

					//kill heliostats which aren't full enough
					for (auto it = heliostatsToAdd.begin(); it != heliostatsToAdd.end();) {
						auto heliostat = *it;
						if (capturesToMove[heliostat].size() < this->parameters.heliostatProjection.minimumCaptures) {
							it = heliostatsToAdd.erase(it);
						}
						else {
							it++;
						}
					}

					//move captures
					for (auto it : capturesToMove) {
						auto heliostat = it.first;
						auto captures = it.second;

						//move the captures we moved from our set to that set
						for (auto capture : captures) {
							//clone by serialization
							nlohmann::json json;
							capture->serialize(json);
							auto captureClone = make_shared<Capture>();
							captureClone->deserialize(json);
							heliostat->captures.add(captureClone);

							capture->onDeletePressed.notifyListeners();
						}

						//add them to the heliostats node if it's in the set
						//if (heliostatsToAdd.find(heliostat) != heliostatsToAdd.end()) {
						//HACK WTF?
							heliostatsNode->add(heliostat);
						//}

						//recalc centers for heliostats
						heliostat->calcPosition(this->parameters.heliostatProjection.planeToA2);
					}
				}

				//----------
				void BoardOnMirror::serialize(nlohmann::json & json) {
					this->captures.serialize(json["captures"]);
				}

				//----------
				void BoardOnMirror::deserialize(const nlohmann::json & json) {
					this->captures.deserialize(json["captures"]);
				}

#pragma mark Capture
				//----------
				BoardOnMirror::Capture::Capture() {
					RULR_SERIALIZE_LISTENERS;
				}

				//----------
				void BoardOnMirror::Capture::drawWorld() {
					this->drawPlane(this->mirrorPlane);

					//draw the mirror
					ofPushMatrix();
					{
						ofQuaternion rotation;
						rotation.makeRotate(glm::vec3(0, 0, 1), this->mirrorPlane.plane.getNormal());

						ofTranslate(this->mirrorPlane.plane.getCenter());
						ofMultMatrix(glm::mat4((glm::quat) rotation));

						ofPushStyle();
						{
							ofSetColor(this->color);
							ofNoFill();
							ofDrawCircle(glm::vec3(), 0.35f / 2.0f);
						}
						ofPopStyle();
					}
					ofPopMatrix();

					ofVboMesh mesh;
					mesh.setMode(OF_PRIMITIVE_LINES);

					//draw the rays for camera navigation
					if (this->cameraNavigation.enabled) {
						for (const auto & worldPoint : this->cameraNavigation.worldPoints) {
							auto color = (ofFloatColor) this->color.get();
							mesh.addVertex(this->cameraNavigation.cameraPosition);
							mesh.addColor(color);

							color.a = 0.0f;
							mesh.addVertex(ofxCv::toOf(worldPoint));
							mesh.addColor(color);
						}
					}

					//draw the ray towards the test point where it lands on the mirror plane
					{
						mesh.addVertex(this->cameraNavigation.cameraPosition);
						mesh.addColor(this->color.get());

						mesh.addVertex(this->mirrorPlane.meanWorldPoint);
						mesh.addColor(this->color.get());
					}

					//draw the name if any
					if (!this->name.empty()) {
						ofDrawBitmapString(name, this->mirrorPlane.meanWorldPoint);
					}

					ofPushStyle();
					{
						ofEnableAlphaBlending();
						mesh.draw();
					}
					ofPopStyle();
				}

				//----------
				std::string BoardOnMirror::Capture::getDisplayString() const {
					stringstream message;
					if (!this->name.empty()) {
						message << this->name << endl;
					}

					if (this->cameraNavigation.enabled) {
						message << "Marker Map reprojection error : " << this->cameraNavigation.reprojectionError << endl;
					}
					message << "'Board reprojection error : " << this->mirrorPlane.reprojectionError << endl;
					return message.str();
				}

				//----------
				void BoardOnMirror::Capture::serialize(nlohmann::json & json) {
					json["name"] << this->name;
					json["heliostatName"] << this->heliostatName;

					{
						auto & jsonCameraNavigation = json["cameraNavigation"];
						Utils::serialize(jsonCameraNavigation["enabled"], this->cameraNavigation.enabled);
						Utils::serialize(jsonCameraNavigation["imagePoints"], ofxCv::toOf(this->cameraNavigation.imagePoints));
						Utils::serialize(jsonCameraNavigation["worldPoints"], ofxCv::toOf(this->cameraNavigation.worldPoints));
						Utils::serialize(jsonCameraNavigation["cameraPosition"], this->cameraNavigation.cameraPosition);
						Utils::serialize(jsonCameraNavigation, this->cameraNavigation.reprojectionError);
					}

					{
						auto & jsonPlane = json["mirrorPlane"];
						Utils::serialize(jsonPlane["imagePoints"], ofxCv::toOf(this->mirrorPlane.imagePoints));
						Utils::serialize(jsonPlane["worldPoints"], ofxCv::toOf(this->mirrorPlane.objectPoints));
						Utils::serialize(jsonPlane["plane"], this->mirrorPlane.plane);
						Utils::serialize(jsonPlane["reprojectionError"], this->mirrorPlane.reprojectionError);
						Utils::serialize(jsonPlane["planeFitResidual"], this->mirrorPlane.planeFitResidual);
						Utils::serialize(jsonPlane["boardTransform"], this->mirrorPlane.boardTransform);
						Utils::serialize(jsonPlane["worldPoints"], this->mirrorPlane.worldPoints);
						Utils::serialize(jsonPlane["meanObjectPoint"], this->mirrorPlane.meanObjectPoint);
						Utils::serialize(jsonPlane["meanWorldPoint"], this->mirrorPlane.meanWorldPoint);
					}
				}

				//----------
				void BoardOnMirror::Capture::deserialize(const nlohmann::json & json) {
					json["name"] >> this->name;
					json["heliostatName"] >> this->heliostatName;

					{
						const auto & jsonCameraNavigation = json["cameraNavigation"];
						this->cameraNavigation.enabled = jsonCameraNavigation["enabled"].get<bool>();
						Utils::deserialize(jsonCameraNavigation["imagePoints"], ofxCv::toOf(this->cameraNavigation.imagePoints));
						Utils::deserialize(jsonCameraNavigation["worldPoints"], ofxCv::toOf(this->cameraNavigation.worldPoints));
						Utils::deserialize(jsonCameraNavigation["cameraPosition"], this->cameraNavigation.cameraPosition);
						Utils::deserialize(jsonCameraNavigation, this->cameraNavigation.reprojectionError);
					}

					{
						const auto & jsonPlane = json["mirrorPlane"];
						Utils::deserialize(jsonPlane["imagePoints"], ofxCv::toOf(this->mirrorPlane.imagePoints));
						Utils::deserialize(jsonPlane["worldPoints"], ofxCv::toOf(this->mirrorPlane.objectPoints));
						Utils::deserialize(jsonPlane["plane"], this->mirrorPlane.plane);
						Utils::deserialize(jsonPlane["reprojectionError"], this->mirrorPlane.reprojectionError);
						Utils::deserialize(jsonPlane["planeFitResidual"], this->mirrorPlane.planeFitResidual);
						Utils::deserialize(jsonPlane["boardTransform"], this->mirrorPlane.boardTransform);
						Utils::deserialize(jsonPlane["worldPoints"], this->mirrorPlane.worldPoints);
						Utils::deserialize(jsonPlane["meanObjectPoint"], this->mirrorPlane.meanObjectPoint);
						Utils::deserialize(jsonPlane["meanWorldPoint"], this->mirrorPlane.meanWorldPoint);
					}
				}

				//----------
				void BoardOnMirror::Capture::drawPlane(const Plane & plane) const {
					ofPushMatrix();
					{
						ofMultMatrix(plane.boardTransform);
						ofDrawAxis(0.1f);
					}
					ofPopMatrix();

					ofPushStyle();
					{
						ofSetColor(this->color);
						ofxCv::drawCorners(plane.worldPoints, false);
					}
					ofPopStyle();
				}
			}
		}
	}
}
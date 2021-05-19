#include "pch_Plugin_Experiments.h"

template<typename T>
T mean(const vector<T> & points) {
	T accumulator(0);

	for (const auto & point : points) {
		accumulator += point;
	}
	return accumulator / size(points);
}

namespace ofxRulr {
	namespace Nodes {
		namespace Experiments {
			namespace MirrorPlaneCapture {
				//----------
				BoardInMirror::BoardInMirror() {
					RULR_NODE_INIT_LISTENER;
				}

				//----------
				string BoardInMirror::getTypeName() const {
					return "Halo::BoardInMirror";
				}

				//----------
				void BoardInMirror::init() {
					RULR_NODE_DRAW_WORLD_LISTENER;
					RULR_NODE_INSPECTOR_LISTENER;
					RULR_NODE_UPDATE_LISTENER;

					this->addInput<Item::Camera>();
					this->addInput<Item::AbstractBoard>();
					this->addInput<ArUco::Detector>();
					this->addInput<ArUco::MarkerMap>();

					this->panel = make_shared<ofxCvGui::Panels::Widgets>();
					this->captures.populateWidgets(this->panel);

					this->manageParameters(this->parameters);
				}

				//----------
				void BoardInMirror::update() {

				}

				//----------
				void BoardInMirror::addCapture() {
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
				void BoardInMirror::calibrate() {

				}

				//----------
				ofxCvGui::PanelPtr BoardInMirror::getPanel() {
					return this->panel;
				}

				//----------
				void BoardInMirror::populateInspector(ofxCvGui::InspectArguments & inspectArgs) {
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
				}

				//----------
				void BoardInMirror::drawWorldStage() {
					auto captures = this->captures.getSelection();
					for (auto capture : captures) {
						capture->drawWorld();
					}
				}

				//----------
				void BoardInMirror::addImage(const cv::Mat & image, const string & name) {
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

						auto markers = detector->getMarkerDetector().detect(image);
						if (markers.size() < this->parameters.cameraNavigation.minimumMarkers) {
							throw(ofxRulr::Exception("Found " + ofToString(markers.size()) + " real markers. Need " + ofToString(this->parameters.cameraNavigation.minimumMarkers) + " for camera navigation"));
						}

						//build up dataset
						vector<cv::Point3f> worldPoints;
						vector<cv::Point2f> imagePoints;
						{
							for (const auto & marker : markers) {
								try {
									auto marker3D = markerMap->getMarker3DInfo(marker.id);
									for (int i = 0; i < 4; i++) {
										worldPoints.push_back(marker3D.points[i]);
										imagePoints.push_back(marker[i]);
									}
								}
								RULR_CATCH_ALL_TO_ERROR; //in the case that the marker did not exist
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
					auto findBoardPlane = [this, camera, board, &getReprojectionError, &cameraTransform](const cv::Mat & image, Capture::Plane & plane, bool mirrored) {

						auto spaceName = string(mirrored ? "mirrored" : "real");
						//flip image if needed
						cv::Mat image2;
						if (mirrored) {
							cv::flip(image, image2, 0);
						}
						else {
							image2 = image;
						}

						//get board image and object space points
						if (!board->findBoard(image2
							, plane.imagePoints
							, plane.objectPoints
							, this->parameters.capture.findBoardMode
							, camera->getCameraMatrix()
							, camera->getDistortionCoefficients())) {
							throw(Exception("Failed to find board in " + spaceName + " space"));
						}

						//check we found enough corners
						if (plane.objectPoints.size() < this->parameters.capture.minimumCorners) {
							throw(Exception("Not enough corners found in " + spaceName + " space (" + ofToString(plane.objectPoints.size()) + " found. Needs " + ofToString(this->parameters.capture.minimumCorners) + ")"));
						}

						//flip result if needed
						if (mirrored) {
							//flip the image
							for (auto & imagePoint : plane.imagePoints) {
								imagePoint.y = camera->getHeight() - imagePoint.y; // not -1 again because subpixel
							}

							//flip the object points
							for (auto & objectPoint : plane.objectPoints) {
								objectPoint.y *= -1.0f;
							}
						}

						cv::Mat rotation, translation;
						{
							float acceptableReprojectionError = ofClamp(camera->getWidth() / 1000, 1, 10);

							//first pass EPNP
							cv::solvePnPRansac(plane.objectPoints
								, plane.imagePoints
								, camera->getCameraMatrix()
								, camera->getDistortionCoefficients()
								, rotation
								, translation
								, false
								, 100
								, acceptableReprojectionError
								, 0.99
								, cv::noArray()
								, cv::SOLVEPNP_EPNP);

							//second pass ITERATIVE
							cv::solvePnPRansac(plane.objectPoints
								, plane.imagePoints
								, camera->getCameraMatrix()
								, camera->getDistortionCoefficients()
								, rotation
								, translation
								, true
								, 100
								, acceptableReprojectionError
								, 0.99
								, cv::noArray()
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
							throw(Exception("Failed to fit board plane to points in " + spaceName + "space"));
						}

						//take mean of object space points
						plane.meanObjectPoint = mean(ofxCv::toOf(plane.objectPoints));
					};
					//
					//--



					//find board plane in real space
					findBoardPlane(image, capture->realPlane, false);

					//find board plane in virtual space
					findBoardPlane(image, capture->virtualPlane, true);

					//solve mirror plane
					{
						//choose an object point to use
						capture->testObjectPoint = (capture->realPlane.meanObjectPoint + capture->virtualPlane.meanObjectPoint * glm::vec3(1, -1, 1)) / 2.0f;

						//get the test points in real and virtual planes
						capture->realPlane.testPointWorld = Utils::applyTransform(capture->realPlane.boardTransform, capture->testObjectPoint);
						capture->virtualPlane.testPointWorld = Utils::applyTransform(capture->virtualPlane.boardTransform, capture->testObjectPoint * glm::vec3(1, -1, 1));

						capture->mirrorPlane = ofxRay::Plane();
						capture->mirrorPlane.setInfinite(true);
						capture->mirrorPlane.setCenter((capture->realPlane.testPointWorld + capture->virtualPlane.testPointWorld) / 2.0f);
						capture->mirrorPlane.setNormal(glm::normalize(capture->realPlane.testPointWorld - capture->virtualPlane.testPointWorld));


						//where is the test point reflected?
						{
							const auto view = camera->getViewInWorldSpace();

							//ray to test point in reflection
							auto rayToReflectionPoint = ofxRay::Ray(camera->getPosition()
								, glm::vec3()
								, capture->color
								, false);

							rayToReflectionPoint.setEnd(capture->virtualPlane.testPointWorld);

							if (!capture->mirrorPlane.intersect(rayToReflectionPoint, capture->estimatedPointOnMirrorPlane)) {
								throw(new Exception("Ray towards reflected test point does not intersect with mirror plane"));
							}

							capture->mirrorPlane.setCenter(capture->estimatedPointOnMirrorPlane);
						}

						// for visualization purposes
						capture->mirrorPlane.setScale({ 1.0f, 1.0f });
						capture->mirrorPlane.setUp({ 0.0f, 1.0f, 1.0f });
						capture->mirrorPlane.setInfinite(false);
						capture->mirrorPlane.color = capture->color;
					}

					//log to server
					if (this->parameters.logToServer.enabled) {
						try {
							Utils::ScopedProcess scopedProcessLogToServer("Log to server");
							nlohmann::json requestJson;

							{
								auto & data = requestJson["data"];

								auto & jsonPlane = data["plane"];

								jsonPlane["ABCD"] << capture->mirrorPlane.getABCD();
								jsonPlane["center"] << capture->mirrorPlane.getCenter();
								jsonPlane["normal"] << capture->mirrorPlane.getNormal();
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
				void BoardInMirror::addFolderOfImages(const std::filesystem::path & path) {
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

#pragma mark Capture
				//----------
				void BoardInMirror::Capture::drawWorld() {
					this->drawPlane(this->realPlane);
					this->drawPlane(this->virtualPlane);

					ofPushStyle();
					{
						ofSetColor(100, 100, 255);
						ofDrawSphere(this->realPlane.testPointWorld, 0.01);

						ofSetColor(100, 255, 100);
						ofDrawSphere(this->virtualPlane.testPointWorld, 0.01);
					}
					ofPopStyle();

					//draw the mirror
					ofPushMatrix();
					{
						ofQuaternion rotation;
						rotation.makeRotate(glm::vec3(0, 0, 1), this->mirrorPlane.getNormal());

						ofTranslate(this->mirrorPlane.getCenter());
						ofMultMatrix(glm::mat4((glm::quat)rotation));

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

						mesh.addVertex(this->estimatedPointOnMirrorPlane);
						mesh.addColor(this->color.get());
					}

					//draw the name if any
					if (!this->name.empty()) {
						ofDrawBitmapString(name, this->estimatedPointOnMirrorPlane);
					}

					ofPushStyle();
					{
						ofEnableAlphaBlending();
						mesh.draw();
					}
					ofPopStyle();
				}

				//----------
				std::string BoardInMirror::Capture::getDisplayString() const {
					stringstream message;
					if (!this->name.empty()) {
						message << this->name << endl;
					}

					if (this->cameraNavigation.enabled) {
						message << "Marker Map reprojection error : " << this->cameraNavigation.reprojectionError << endl;
					}
					message << "Real reprojection error : " << this->realPlane.reprojectionError << endl;
					message << "Virtual reprojection error : " << this->virtualPlane.reprojectionError << endl;
					return message.str();
				}

				//----------
				void BoardInMirror::Capture::drawPlane(const Plane & plane) const {
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
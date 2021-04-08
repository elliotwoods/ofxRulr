#include "pch_Plugin_Experiments.h"

namespace ofxRulr {
	namespace Nodes {
		namespace Experiments {
			namespace SolveMirror {
				//----------
				SolveMirror::SolveMirror() {
					RULR_NODE_INIT_LISTENER;
				}

				//----------
				string SolveMirror::getTypeName() const {
					return "Experiments::SolveMirror::SolveMirror";
				}

				//----------
				void SolveMirror::init() {
					RULR_NODE_DRAW_WORLD_LISTENER;
					RULR_NODE_INSPECTOR_LISTENER;
					RULR_NODE_UPDATE_LISTENER;

					this->addInput<Item::Camera>();
					this->addInput<ArUco::Detector>();
					this->addInput<ArUco::MarkerMap>();
					
					this->panel = make_shared<ofxCvGui::Panels::Widgets>();
					this->captures.populateWidgets(this->panel);
				}

				//----------
				void SolveMirror::update() {

				}

				//----------
				void SolveMirror::addCapture() {
					this->throwIfMissingAnyConnection();
					auto camera = this->getInput<Item::Camera>();
					auto grabber = camera->getGrabber();
					if (!grabber) {
						throw(ofxRulr::Exception("Camera grabber not available"));
					}
					auto frame = grabber->getFreshFrame();
					auto image = ofxCv::toCv(frame->getPixels());
					auto detector = this->getInput<ArUco::Detector>();
					auto markerMap = this->getInput<ArUco::MarkerMap>()->getMarkerMap();

					auto capture = make_shared<Capture>();

					auto getReprojectionError = [& camera](const vector<cv::Point3f> & objectPoints
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

						for (size_t i = 0; i < imagePoints.size(); i++) {
							auto delta = imagePoints[i] - reprojectedPoints[i];
							error += delta.x * delta.x + delta.y * delta.y;
						}
						error /= (float)imagePoints.size();
						error = sqrt(error);
						return error;
					};

					//resolve pose of camera
					{
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

						cv::solvePnPRansac(worldPoints
							, imagePoints
							, camera->getCameraMatrix()
							, camera->getDistortionCoefficients()
							, rotationVector
							, translation
							, true);

						camera->setExtrinsics(rotationVector, translation, true);

						capture->cameraNavigation.imagePoints = imagePoints;
						capture->cameraNavigation.worldPoints = worldPoints;
						capture->cameraNavigation.reprojectionError = getReprojectionError(worldPoints
							, imagePoints
							, rotationVector
							, translation);
						cout << "capture->cameraNavigation.reprojectionError " << capture->cameraNavigation.reprojectionError << endl;
					}

					//get the image space points of mirrored markers
					vector<aruco::Marker> mirroredMarkers;
					{
						//flip the image
						cv::Mat imageFlipped;
						cv::flip(image, imageFlipped, 0);

						//detect the markers
						mirroredMarkers = detector->getMarkerDetector().detect(imageFlipped);

						if (mirroredMarkers.empty()) {
							throw(ofxRulr::Exception("No mirrored markers found"));
						}

						//flip the markers back
						for (auto & marker : mirroredMarkers) {
							for (int i = 0; i < 4; i++) {
								marker[i].y = grabber->getHeight() - marker[i].y; //not -1 again, because we're subpixels
							}
						}
					}

					//get the object points in mirrored space
					vector<cv::Point3f> flippedMarkerMapWorldSpacePoints;
					vector<cv::Point2f> reflectionImageSpacePoints;
					{
						for (const auto & marker : mirroredMarkers) {
							try {
								auto markerInfo = markerMap->getMarker3DInfo(marker.id);

								for (const auto & point : markerInfo.points) {
									capture->reflections.realPoints.push_back(point);
									auto pointInverted = point;
									pointInverted.y *= -1.0f;
									flippedMarkerMapWorldSpacePoints.push_back(pointInverted);
								}

								reflectionImageSpacePoints.push_back(marker[0]);
								reflectionImageSpacePoints.push_back(marker[1]);
								reflectionImageSpacePoints.push_back(marker[2]);
								reflectionImageSpacePoints.push_back(marker[3]);
							}
							catch (...) {
								ofLogNotice("SolveMirror") << "Couldn't find Marker #" << marker.id << "In the marker map. Ignoring.";
							}
						}
					}

					//resolve pose of inverted marker map relative to camera
					glm::mat4 flippedMarkerMapTransform;
					{
						cv::Mat rotation, translation;
						cv::solvePnP(flippedMarkerMapWorldSpacePoints
							, reflectionImageSpacePoints
							, camera->getCameraMatrix()
							, camera->getDistortionCoefficients()
							, rotation
							, translation);

						capture->reflections.reprojectionError = getReprojectionError(flippedMarkerMapWorldSpacePoints
							, reflectionImageSpacePoints
							, rotation
							, translation);
						cout << "capture->reflections.reprojectionError " << capture->reflections.reprojectionError << endl;

						flippedMarkerMapTransform = ofxCv::makeMatrix(rotation, translation) * camera->getTransform();
					}

					//resolve 3d points as seen by camera through mirror
					{
						for (const auto & flippedWorldSpacePoint : flippedMarkerMapWorldSpacePoints) {
							capture->reflections.virtualPoints.push_back(
								ofxCv::toCv(Utils::applyTransform(flippedMarkerMapTransform, ofxCv::toOf(flippedWorldSpacePoint))));
						}
					}

					//make view rays
					{
						for (const auto & virtualPoint : capture->reflections.virtualPoints) {
							ofxRay::Ray ray;
							ray.infinite = false;
							ray.defined = true;
							ray.s = camera->getPosition();
							ray.setEnd(ofxCv::toOf(virtualPoint));
							ray.color = capture->color;
							capture->reflections.viewRays.push_back(ray);
						}
					}

					this->captures.add(capture);
				}

				//----------
				void SolveMirror::calibrate() {
					vector<PlaneDataPoint> dataPoints;
					auto captures = this->captures.getSelection();
					for (auto capture : captures) {
						for (int i = 0; i < capture->reflections.realPoints.size(); i++) {
							dataPoints.push_back(PlaneDataPoint {
								ofxCv::toOf(capture->reflections.realPoints[i])
								, ofxCv::toOf(capture->reflections.virtualPoints[i])
							});
						}
					}

					ofxNonLinearFit::Fit<PlaneModel> fit;
					double fitResidual;
					fit.optimise(this->mirrorPlaneModel, &dataPoints, & fitResidual);
					this->planeFitResidual = sqrt(fitResidual);
					
					auto & plane = this->mirrorPlaneModel.plane;
					plane.setInfinite(true);

					//calculate plane intersection points
					glm::vec3 meanPlaneIntersection;
					{
						size_t intersectionCount = 0;

						for (auto capture : captures) {
							capture->reflections.pointOnMirror.clear();

							for (auto & viewRay : capture->reflections.viewRays) {
								glm::vec3 position;
								plane.intersect(viewRay, position);
								capture->reflections.pointOnMirror.push_back(ofxCv::toCv(position));
								viewRay.setEnd(position);
								meanPlaneIntersection += position;
								intersectionCount++;
							}
						}

						meanPlaneIntersection /= intersectionCount;
						plane.setCenter(meanPlaneIntersection);
						plane.setScale(glm::vec2(0.35f, 0.35f));
						plane.setInfinite(false);
					}

				}

				//----------
				ofxCvGui::PanelPtr SolveMirror::getPanel() {
					return this->panel;
				}

				//----------
				void SolveMirror::populateInspector(ofxCvGui::InspectArguments & inspectArgs) {
					auto inspector = inspectArgs.inspector;
					inspector->addButton("Add capture", [this]() {
						try {
							this->addCapture();
						}
						RULR_CATCH_ALL_TO_ALERT;
					}, ' ')->setHeight(100.0f);
					inspector->addButton("Calibrate", [this]() {
						try {
							this->calibrate();
						}
						RULR_CATCH_ALL_TO_ALERT;
					}, OF_KEY_RETURN)->setHeight(100.0f);
					inspector->addLiveValue<float>(this->planeFitResidual);
				}

				//----------
				void SolveMirror::drawWorldStage() {
					auto captures = this->captures.getSelection();
					for (auto capture : captures) {
						capture->drawWorld();
					}

					this->mirrorPlaneModel.plane.draw();
				}

#pragma mark Capture
				//----------
				void SolveMirror::Capture::drawWorld() {
					ofPushStyle();
					{
						ofSetColor(this->color);
						for (size_t i = 0; i < this->reflections.realPoints.size(); i++) {
							ofDrawLine(ofxCv::toOf(this->reflections.realPoints[i])
								, ofxCv::toOf(this->reflections.virtualPoints[i]));
						}
					}
					ofPopStyle();

					for (const auto & viewRay : this->reflections.viewRays) {
						viewRay.draw();
					}
				}

				//----------
				std::string SolveMirror::Capture::getDisplayString() const {
					return "markers";
				}

#pragma mark PlaneModel
				//----------
				unsigned int SolveMirror::PlaneModel::getParameterCount() const {
					return 3;
				}

				//----------
				void SolveMirror::PlaneModel::getResidual(PlaneDataPoint dataPoint, double & residual, double * gradient /* = 0 */) const {
					if (gradient) {
						throw(ofxRulr::Exception("Gradient not supported for this model"));
					}

					auto evaluatedDataPoint = dataPoint;
					this->evaluate(evaluatedDataPoint);
					residual = glm::distance2(evaluatedDataPoint.virtualPosition, dataPoint.virtualPosition);
				}

				//----------
				void SolveMirror::PlaneModel::evaluate(PlaneDataPoint & dataPoint) const {
					dataPoint.virtualPosition = this->plane.reflect(dataPoint.realPosition);
				}

				//----------
				void SolveMirror::PlaneModel::cacheModel() {
					glm::vec3 normal(0, 0, 1);
					glm::mat4 transform;
					{
						ofMatrix4x4 oftransform;
						oftransform.rotate(this->parameters[0], 1, 0, 0);
						oftransform.rotate(this->parameters[1], 0, 1, 0);
						transform = oftransform;
					}
					this->plane.setUp(Utils::applyTransform(transform, glm::vec3(0, 1, 0)));
					this->plane.setScale(glm::vec2(1.0f, 1.0f));
					this->plane.setNormal(Utils::applyTransform(transform, normal));
					this->plane.setCenter(normal * this->parameters[2]);
				}
			}
		}
	}
}
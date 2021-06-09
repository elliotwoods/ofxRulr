#include "pch_Plugin_Experiments.h"

namespace ofxRulr {
	namespace Nodes {
		namespace Experiments {
			namespace MirrorPlaneCapture {				
#pragma mark SunCalibrator
				//---------
				SunCalibrator::SunCalibrator() {
					RULR_NODE_INIT_LISTENER;
				}

				//---------
				string SunCalibrator::getTypeName() const {
					return "Halo::SunCalibrator";
				}

				//---------
				void SunCalibrator::init() {
					RULR_NODE_INSPECTOR_LISTENER;
					RULR_NODE_DRAW_WORLD_LISTENER;

					this->manageParameters(this->parameters);

					this->light.setDirectional();

					this->addInput<MarkerMap::NavigateCamera>();
					this->addInput<SunTracker>();

					this->panel = ofxCvGui::makePanel(this->preview);
					{
						this->panel->onDrawImage += [this](ofxCvGui::DrawImageArguments & args) {
							auto captures = this->captures.getSelection();
							for(auto capture : captures) {
								ofPushMatrix();
								{
									ofTranslate(capture->centroidUndistorted.get());
									ofPushStyle();
									{
										ofSetColor(capture->color);
										ofDrawLine(-5, 0, 5, 0);
										ofDrawLine(0, -5, 0, 5);

										ofPath path;
										path.addVertices(capture->contour);
										path.close();
										ofNoFill();
										path.draw();
									}
									ofPopStyle();
								}
								ofPopMatrix();
							}
						};
					}
				}

				//---------
				void SunCalibrator::drawWorld() {
					auto sunTracker = this->getInput<SunTracker>();
					DrawOptions drawOptions;
					{
						drawOptions.cameraRay = this->parameters.draw.cameraRay.get();
						drawOptions.cameraView = this->parameters.draw.cameraView.get();
						drawOptions.solarVector = this->parameters.draw.solarVector.get();
						if(sunTracker) {
							drawOptions.sunTrackerTransform = sunTracker->getTransform();
						}
					}
					auto captures = this->captures.getSelection();
					for(auto capture : captures) {
						capture->drawWorld();
					}
				}

				//---------
				void SunCalibrator::populateInspector(ofxCvGui::InspectArguments& inspectArgs) {
					auto inspector = inspectArgs.inspector;
					
					this->captures.populateWidgets(inspector);

					inspector->addButton("Add from files", [this]() {
						try {
							Utils::ScopedProcess scopedProcess("Add solar capture file pair");
							this->addFromFiles();
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

				//---------
				void SunCalibrator::serialize(nlohmann::json & json) const {
					this->captures.serialize(json["captures"]);
				}

				//---------
				void SunCalibrator::deserialize(nlohmann::json & json){
					if(json.contains("captures")) {
						this->captures.deserialize(json["captures"]);
					}
				}

				//---------
				void SunCalibrator::addFromFiles() {
					this->throwIfMissingAConnection<MarkerMap::NavigateCamera>();
					auto navigateCameraNode = this->getInput<MarkerMap::NavigateCamera>();
					navigateCameraNode->throwIfMissingAConnection<Item::Camera>();
					auto camera = navigateCameraNode->getInput<Item::Camera>();

					this->throwIfMissingAConnection<SunTracker>();
					auto sunTracker = this->getInput<SunTracker>();

					// Ask for UNIX timestamp (i.e. don't trust the file times)
					chrono::system_clock::time_point timestamp;
					{
						uint64_t epochTimestamp;
						{
							auto timeString = ofSystemTextBoxDialog("Milliseconds since UNIX epoch for capture");
							if(timeString.empty()) {
								return;
							}
							std::istringstream iss(timeString);
							iss >> epochTimestamp;
						}

						timestamp = chrono::system_clock::time_point{chrono::milliseconds{epochTimestamp}};
					}

					// Navigate camera
					{
						auto result = ofSystemLoadDialog("Select navigation image (WARNING : Pre-process the RAW using toolMonoDebayer)");
						if (!result.bSuccess) {
							return;
						}
						auto navigationImage = cv::imread(result.path);
						if(navigationImage.getDepth() == cv::CV_16U) {
							navigationImage.convertTo(navigationImage, cv::CV_8U);
						}
						navigateCameraNode->track(navigationImage);
					}

					// Find sun in camera
					{
						auto result = ofSystemLoadDialog("Select solar capture (WARNING : Pre-process the RAW using toolMonoDebayer)");
						if (!result.bSuccess) {
							return;
						}
 						
						ofShortPixels rawPixels;

						// Load raw as-is
						ofLoadImage(rawPixels
							, result.path
							, imageLoadSettings);
						
						// Down-convert to 8-bit
						this->preview.getPixels() = rawPixels;

						cv::Mat image = ofxCv::toCv(this->preview.getPixels());
						
						// Threshold to binary image
						cv::Mat binary;
						{
							cv::threshold(image
								, binary
								, this->parameters.capture.threshold.get()
								, 255
								, cv::THRESH_BINARY);
						}

						// Find contours
						vector<vector<cv::Point2i>> contours;
						{
							cv::findContours(binary
								, contours
								, cv::RETR_EXTERNAL
								, cv::CHAIN_APPROX_NONE);
							if (contours.size() < 1) {
								throw(ofxRulr::Exception("Found 0 contours. Need 1"));
							}
						}

						vector<cv::Point2f> centroids;
						vector<cv::Rect> boundingRects;
						vector<cv::Moments> moments;

						// Find the best contour
						vector<cv::Point2i> bestContour;
						{
							float bestArea = 0;
							for (const auto & contour : contours) {
								auto bounds = cv::boundingRect(contour);
								auto area = bounds.area();
								if (area > bestArea) {
									bestContour = contour;
									bestArea = area;
								}
							}
						}

						// Get some bounds around the contour
						ofRectangle bounds;
						{
							bounds = ofxCv::toOf(cv::boundingRect(bestContour));
							bounds.x -= 5;
							bounds.y -= 5;
							bounds.width += 10;
							bounds.height += 10;

							// Check if bounds stretch outside of image area
							if(bounds.x < 0
								|| bounds.y < 0
								|| bounds.getRight() >= image.cols
								|| bounds.getBottom() >= image.rows) {
									throw(ofxRulr::Exception("Bounds overlaps edge of image"));
								}
						}

						// Get the centroid
						glm::vec2 centroid;
						{
							auto moment = cv::moments(image(ofxCv::toCv(bounds)));
							centroid = glm::vec2(moment.m10 / moment.m00 + bounds.x
								, moment.m01 / moment.m00 + bounds.y);
						}

						// Undistort the centroid
						glm::vec2 centroidUndistorted;
						{
							vector<cv::Point2f> imagePointsDistorted(1, centroid);
							auto imagePointsUndistorted = ofxCv::undistortImagePoints(imagePointsDistorted
								, camera->getCameraMatrix()
								, camera->getDistortionCoefficients());
							centroidUndistorted = imagePointsUndistorted[0];
						}

						// Get the camera ray vector
						auto cameraView = camera->getViewInWorldSpace();
						auto cameraRay = cameraView.castPixel(centroidUndistorted);
						{
							// Shorten the camera view for preview
							cameraView.setFarClip(0.1f);
							cameraRay.setLength(1.0f);
						}

						// Get the solar vector in object space for this time
						auto solarVectorObjectSpace = sunTracker->getSolarVectorObjectSpace(timestamp);

						// Make and store the capture
						auto capture = make_shared<Capture>();
						{
							capture->centroidUndistorted.set(centroidUndistorted);
							capture->solarVectorObjectSpace.set(solarVectorObjectSpace);
							capture->cameraRay = cameraRy;
							capture->cameraRay.color = capture->color;
							capture->contour = ofxCv::toOf(contour);
							capture->bounds = bounds;
							capture->timestamp.set(timestamp);
							capture->rebuildDateStrings();
						}
						this->captures.add(capture);
					}
				}

				//---------
				void SunCalibrator::calibrate() {
					this->throwIfMissingAConnection<SunTracker>();

					auto captures = this->captures.getSelection();
					if(captures.size() < 2) {
						throw(ofxRulr::Exception("Minimum 2 captures required"));
					}

					// Gather data for solver
					vector<glm::vec3> preTransformVectors;
					vector<glm::vec3> postTransformVectors;
					{
						for(auto capture : captures) {
							preTransformVectors.push_back(capture->solarVectorObjectSpace.get());
							postTransformVectors.push_back(glm::normalize(-capture->cameraRay.t));
						}
					}

					// Prepare to solve
					auto solverSettings = Solvers::RotationFrame::defaultSolverSettings();
					{
						solverSettings.printOutput = this->parameters.solver.printOutput.get();
						solverSettings.options.functionTolerance = this->parameters.solver.functionTolerance.get();
						solverSettings.options.num_max_iterations = this->parameters.solver.maxIterations.get();
					}

					// Perform the solve
					auto result = RotationFrame::solve(preTransformVectors
						, postTransformVectors
						, solverSettings);

					sunTracker->setOrientation(result.rotation);	
				}

#pragma mark Capture
				//---------
				void SunCalibrator::Capture::Capture() {
					RULR_SERIALIZE_LISTENERS;
				}

				//---------
				string SunCalibrator::Capture::getDisplayString() const {
					stringstream ss;
					ss << "(" << this->centroid.get() <<< ") = (" << this->solarVectorObjectSpace.get() << ")";
				}

				//---------
				void SunCalibrator::Capture::drawWorld(const DrawOptions& drawOptions) {
					if(drawOptions.solarVector) {
						ofPushMatrix();
						{
							ofMultMatrix(drawOptions.sunTrackerTransform);
							ofPushStyle();
							{
								ofSetColor(this->color);
								ofDrawArrow(- this->solarVector.get()
								, {0, 0, 0}
								, 0.1f);
							}
							ofPopStyle();	
						}
						ofPopMatrix();
					}

					if(drawOptions.cameraRay) {
						this->cameraRay.draw();
					}

					if(drawOptions.cameraView) {
						this->cameraView.draw();
					}
				}

				//---------
				void SunCalibrator::Capture::serialize(nlohmann::json & json) const {
					Utils::serialize(json, this->centroidUndistorted);
					Utils::serialize(json, this->cameraVector);
					Utils::serialize(json, this->solarVectorObjectSpace);
					Utils::serialize(json, "cameraRay", this->cameraRay);
					Utils::serialize(json, "contour", this->contour);
					Utils::serialize(json, "bounds", this->bounds);
				}

				//---------
				void SunCalibrator::Capture::deserialize(const nlohmann::json & json) {
					Utils::deserialize(json, this->centroidUndistorted);
					Utils::deserialize(json, this->cameraVector);
					Utils::deserialize(json, this->solarVectorObjectSpace);
					Utils::deserialize(json, "cameraRay", this->cameraRay);
					Utils::deserialize(json, "contour", this->contour);
					Utils::deserialize(json, "bounds", this->bounds);
				}

				//---------
				ofxCvGui::ElementPtr SunCalibrator::Capture::getDataDisplay() {
					auto element = ofxCvGui::makeElement();

					vector<ofxCvGui::ElementPtr> widgets;

					widgets.push_back(make_shared<ofxCvGui::Widgets::EditableValue<glm::vec2>>(this->centroidUndistorted));
					widgets.push_back(make_shared<ofxCvGui::Widgets::EditableValue<glm::vec3>>(this->cameraVector));
					widgets.push_back(make_shared<ofxCvGui::Widgets::EditableValue<glm::vec3>>(this->solarVectorObjectSpace));

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
			}
		}
	}
}
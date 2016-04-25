#include "pch_MultiTrack.h"
#include "Calibrate.h"

#include "ofxRulr/Utils/ScopedProcess.h"

#include "ofxCvGui/Widgets/Button.h"

#include "ofxRulr/Nodes/MultiTrack/World.h"

#include "ofxMultiTrack/Receiver.h"

namespace ofxRulr {
	namespace Nodes {
		namespace MultiTrack {
			namespace Procedure {
				//----------
				Calibrate::Calibrate() {
					RULR_NODE_INIT_LISTENER;
				}

				//----------
				string Calibrate::getTypeName() const {
					return "MultiTrack::Procedure::Calibrate";
				}

				//----------
				void Calibrate::init() {
					RULR_NODE_UPDATE_LISTENER;
					RULR_NODE_INSPECTOR_LISTENER;
					RULR_NODE_SERIALIZATION_LISTENERS;
					RULR_NODE_DRAW_WORLD_LISTENER;

					this->currStep = StepIdle;

					this->panel = ofxCvGui::Panels::makeWidgets();
					auto button = this->panel->addButton("Open Capture", [this]() {
						this->goToStep(StepBegin);
					});

					//Only draw the button when a World is connected.
					auto world = this->getInput<World>();
					button->setEnabled(world.get());

					auto worldPin = this->addInput<ofxRulr::Nodes::MultiTrack::World>();
					worldPin->onNewConnection += [this, button](shared_ptr<World> world) {
						button->setEnabled(true);
					};
					worldPin->onDeleteConnection += [this, button](shared_ptr<World> world) {
						button->setEnabled(false);
					};
				}

				//----------
				ofxCvGui::PanelPtr Calibrate::getPanel() {
					return this->panel;
				}

				//----------
				void Calibrate::update() {
					if (this->currStep == StepBegin) {
						this->captureFrame(false);
					}
					else if (this->currStep == StepCapture) {
						this->captureFrame(true);

						if (this->getTimeSinceCaptureStarted() > chrono::seconds(this->parameters.capture.duration)) {
							this->goToStep(StepSolve);
						}
					}
				}

				//----------
				void Calibrate::drawWorld() {
					ofPushStyle();
					for (auto & it : this->dataToPreview) {
						ofSetColor(this->getSubscriberColor(it.first));
						for (auto & m : it.second) {
							ofDrawSphere(m.position, 0.1f);
						}
					}
					ofPopStyle();

					if (!this->parameters.debugWorld.drawLines && !this->parameters.debugWorld.drawPoints) return;

					//Cache the transforms for each subscriber.
					auto world = this->getInput<World>();
					if (!world) return;

					map<size_t, ofMatrix4x4> transforms;
					auto & subscribers = world->getSubscribers();
					for (auto & it : subscribers) {
						auto weak_subscriber = it.second;
						if (!weak_subscriber.expired()) {
							auto subscriberNode = weak_subscriber.lock();
							transforms.emplace(it.first, subscriberNode->getTransform());
						}
					}

					if (this->parameters.debugWorld.drawLines) {
						vector<ofVec3f> linePoints;
						vector<ofFloatColor> colors;
						//Go through subscribers in pairs.
						for (auto & srcIt = this->dataToSolve.begin(); srcIt != this->dataToSolve.end(); ++srcIt) {
							if (transforms.find(srcIt->first) == transforms.end()) break;
							const auto & srcTransform = transforms[srcIt->first];

							//Make sure there is a next element to work with.
							auto & dstIt = next(srcIt);
							if (dstIt != this->dataToSolve.end()) {
								if (transforms.find(dstIt->first) == transforms.end()) break;
								const auto & dstTransform = transforms[dstIt->first];

								auto & srcFrames = srcIt->second;
								auto & dstFrames = dstIt->second;
								for (auto & kIt : srcFrames) {
									auto frameNum = kIt.first;
									//Make sure the frame numbers match.
									if (dstFrames.find(frameNum) != dstFrames.end()) {
										//Add the points.
										auto lineStart = srcFrames[frameNum].position * srcTransform;
										auto lineEnd = dstFrames[frameNum].position * dstTransform;

										linePoints.push_back(lineStart.getInterpolated(lineEnd, 0.01f));
										linePoints.push_back(lineEnd.getInterpolated(lineStart, 0.01f));

										colors.push_back(this->getSubscriberColor(srcIt->first));
										colors.push_back(this->getSubscriberColor(dstIt->first));
									}
								}
							}
						}

						//Draw the VBO.
						ofVbo vbo;
						vbo.setVertexData(linePoints.data(), linePoints.size(), GL_STATIC_DRAW);
						vbo.setColorData(colors.data(), colors.size(), GL_STATIC_DRAW);
						vbo.draw(GL_LINES, 0, linePoints.size());
					}

					if (this->parameters.debugWorld.drawPoints) {
						auto & subscribers = this->getInput<World>()->getSubscribers();
						for (auto & it : subscribers) {
							auto weak_subscriber = it.second;
							if (!weak_subscriber.expired()) {
								auto subscriberNode = weak_subscriber.lock();
								ofMesh mesh;
								for (const auto & jt : this->dataToSolve[it.first]) {
									mesh.addVertex(jt.second.position);
								}

								ofPushMatrix();
								ofPushStyle();
								{
									ofSetColor(this->getSubscriberColor(it.first));
									ofMultMatrix(subscriberNode->getTransform());

									mesh.draw(OF_MESH_POINTS);
								}
								ofPopStyle();
								ofPopMatrix();
							}
						}
					}
				}

				//----------
				void Calibrate::goToStep(Step nextStep) {
					// TODO: Turn this back on once we get a close callback on the Dialog
					//if (this->currStep == nextStep) return;

					this->currStep = nextStep;

					ofxCvGui::closeDialog();
					this->dialog.reset();

					if (this->currStep == StepIdle) return;

					auto addPreviews = [this](shared_ptr<ofxCvGui::Panels::Widgets> & panel, bool history) {
						auto & subscribers = this->getInput<World>()->getSubscribers();
						size_t count = 0;
						shared_ptr<ofxCvGui::Panels::Groups::Strip> strip;
						for (auto & it : subscribers) {
							auto weak_subscriber = it.second;
							if (!weak_subscriber.expired()) {
								auto subscriber = weak_subscriber.lock();

								if (count % 2 == 0) {
									strip = ofxCvGui::Panels::Groups::makeStrip();
									strip->setHeight(360.0f);
									panel->add(strip);
								}

								auto texture = ofxCvGui::Panels::makeTexture(subscriber->getPreviewTexture());
								texture->setHeight(360.0f);
								strip->add(texture);

								auto key = it.first;
								texture->onDrawImage += [this, key, history](ofxCvGui::DrawImageArguments & args) {
									ofPushStyle();
									{
										ofSetColor(this->getSubscriberColor(key));
										
										if (history) {
											const auto & frames = this->dataToSolve[key];
											if (frames.size()) {
												const auto lastFrame = (*frames.rbegin()).first;
												for (const auto & it : frames) {
													ofDrawCircle(it.second.center, MAX(it.second.radius - lastFrame + it.first, it.second.radius * 0.25f));
												}

												ofNoFill();
												for (const auto & it : frames) {
													ofDrawCircle(it.second.center, it.second.radius);
												}
											}
										}
										{
											ofFill();
											const auto & markers = this->dataToPreview[key];
											for (const auto & m : markers) {
												ofDrawCircle(m.center, m.radius);
											}
										}
									}
									ofPopStyle();
								};

								++count;
							}
						}
					};

					shared_ptr<ofxCvGui::Panels::Widgets> panel;
					panel = ofxCvGui::Panels::makeWidgets();
					panel->addTitle("MultiTrack");
					
					if (this->currStep == StepBegin) {
						//GUI.
						panel->addTitle("Step 1: Get Ready!", ofxCvGui::Widgets::Title::Level::H2);
						panel->addTitle("Grab your marker and make sure it's the only red thing in the previews below. Adjust the parameters if necessary.", ofxCvGui::Widgets::Title::Level::H3);

						panel->addParameterGroup(parameters.capture);
						panel->addParameterGroup(parameters.findMarker);

						addPreviews(panel, false);

						auto buttonNext = panel->addButton("Begin Capture", [this]() {
							this->goToStep(StepCapture);
						});
						buttonNext->setHeight(100.0f);
					}
					else if (this->currStep == StepCapture) {
						//Clear previous data.
						this->dataToSolve.clear();
						this->solveSets.clear();

						//Start the timer.
						this->captureStartTime = chrono::system_clock::now(); 
						
						//GUI.
						panel->addTitle("Step 2: Capture", ofxCvGui::Widgets::Title::Level::H2);
						panel->addTitle("Stop reading this and walk across the entire sensor array, making sure the marker is visible.", ofxCvGui::Widgets::Title::Level::H3);

						panel->addLiveValue<float>("Capture remaining [s]", [this]() {
							// Using * 1000.0f so we get floats.
							return (this->parameters.capture.duration * 1000.0f - chrono::duration_cast<chrono::milliseconds>(this->getTimeSinceCaptureStarted()).count()) / 1000.0f;
						});

						addPreviews(panel, true);

						auto buttonNext = panel->addButton("Solve", [this]() {
							this->goToStep(StepSolve);
						});
						buttonNext->setHeight(100.0f);
					}
					else if (this->currStep == StepSolve) {
						//Setup the data set if necessary.
						this->setupSolveSets();

						ofxCvGui::refreshInspector(this);
					}

					if (panel) {
						//Build the Dialog
						auto strip = ofxCvGui::Panels::Groups::makeStrip();
						strip->setDirection(ofxCvGui::Panels::Groups::Strip::Horizontal);

						//Add the common panel.
						{
							auto commonPanel = ofxCvGui::Panels::makeWidgets();

							auto buttonClose = commonPanel->addButton("Close", [this]() {
								this->goToStep(StepIdle);
							});
							buttonClose->setHeight(50.0f);
							commonPanel->addSpacer();
							auto toggleBegin = commonPanel->addToggle("Begin", [this]() {
								return (this->currStep == StepBegin);
							}, [this](bool) {
								this->goToStep(StepBegin);
							});
							toggleBegin->setHeight(50.0f);
							auto toggleCapture = commonPanel->addToggle("Capture", [this]() {
								return (this->currStep == StepCapture);
							}, [this](bool) {
								this->goToStep(StepCapture);
							});
							toggleCapture->setHeight(50.0f);
							auto toggleSolve = commonPanel->addToggle("Solve", [this]() {
								return (this->currStep == StepSolve);
							}, [this](bool) {
								this->goToStep(StepSolve);
							});
							toggleSolve->setHeight(50.0f);

							strip->add(commonPanel);
						}

						//Add the current panel.
						strip->add(panel);

						strip->setCellSizes({ 100, -1 });
						this->dialog = strip;

						//Make sure the dialog is open.
						ofxCvGui::openDialog(this->dialog);
					}
				}

				//----------
				void Calibrate::populateInspector(ofxCvGui::InspectArguments & args) {
					auto inspector = args.inspector;

					//GUI.
					inspector->addTitle("Solve", ofxCvGui::Widgets::Title::Level::H1);

					auto buttonSolve = inspector->addButton("Solve", [this]() {
						try {
							ofxRulr::Utils::ScopedProcess scopedProcess("Solve");

							//Get crackin'
							this->triggerSolvers();

							//Repeat.
							this->goToStep(StepSolve);
							ofxCvGui::refreshInspector(this);

							scopedProcess.end();
						}
						RULR_CATCH_ALL_TO_ALERT;
					});
					buttonSolve->setHeight(100.0f);

					for (auto & it : this->solveSets) {
						auto & result = it.second.getResult();

						inspector->addTitle("Solver " + ofToString(it.first), ofxCvGui::Widgets::Title::Level::H2);

						inspector->addParameterGroup(it.second.parameters);
						inspector->addIndicator("Success", [this, result]() {
							if (result.success) {
								return ofxCvGui::Widgets::Indicator::Status::Good;
							}
							return ofxCvGui::Widgets::Indicator::Status::Error;
						});
						inspector->addLiveValue<float>("Residual", [this, result]() {
							return result.residual;
						});
					}

					auto buttonConfirm = inspector->addButton("Apply", [this]() {
						this->applyTransforms();
					});
					buttonConfirm->setHeight(100.0f);

					panel->addParameterGroup(parameters.debugWorld);
				}

				//----------
				void Calibrate::captureFrame(bool record) {
					size_t frameNum = ofGetFrameNum();
					Marker marker;

					//Get the marker data for this frame.
					auto & subscribers = this->getInput<World>()->getSubscribers();
					for (auto & it : subscribers) {
						auto weak_subscriber = it.second;
						if (!weak_subscriber.expired()) {
							auto subscriberNode = weak_subscriber.lock();
							auto subscriber = subscriberNode->getSubscriber();
							if (subscriber) {
								//if (subscriber->isFrameNew()) {
									//Find the Markers in the frame.
									vector<Marker> markers = findMarkersInFrame(subscriber->getFrame());

									for (auto & marker : markers) {
										//Map the Marker coordinates to world space.
										auto width = subscriber->getFrame().getDepth().getWidth();
										auto depth = subscriber->getFrame().getDepth().getData();
										auto lut = subscriberNode->getDepthToWorldLUT().getData();
										if (lut == nullptr) {
											throw(ofxRulr::Exception("No Depth to World LUT found!"));
										}

										//Sample the area around the 2D marker for an average 3D position.
										ofVec3f avgPos = ofVec3f::zero();
										size_t numFound = 0;
										const float radiusSq = marker.radius * marker.radius;
										for (int y = marker.center.y - marker.radius; y < marker.center.y + marker.radius; ++y) {
											for (int x = marker.center.x - marker.radius; x < marker.center.x + marker.radius; ++x) {
												x = marker.center.x;
												y = marker.center.y;
												if (ofVec2f(x, y).squareDistance(marker.center) <= radiusSq) {
													int idx = y * width + x;
													ofVec3f candidate = ofVec3f(lut[idx * 2 + 0], lut[idx * 2 + 1], 1.0f) * depth[idx] * 0.001f;
													if (candidate != ofVec3f::zero()) {
														avgPos += candidate;
														++numFound;
													}
												}
												break;
											}
											break;
										}
										if (numFound) {
											//The position is valid.

											//Set the marker position to the average.
											avgPos /= numFound;
											marker.position = avgPos;
										}
										else {
											//The position is invalid, ignore it.
											marker.position = ofVec3f::zero();
										}
									}

									//Keep all found Markers for preview, don't bother mapping to 3D.
									if (this->dataToPreview.find(it.first) == this->dataToPreview.end()) {
										this->dataToPreview.emplace(it.first, markers);
									}
									else {
										this->dataToPreview[it.first] = markers;
									}

									if (record) {
										if (markers.size() == 1) {
											auto & marker = markers.front();

											//Map the Marker coordinates to world space.
											auto height = subscriber->getFrame().getDepth().getHeight();
											auto depth = subscriber->getFrame().getDepth().getData();
											auto lut = subscriberNode->getDepthToWorldLUT().getData();
											if (lut == nullptr) {
												throw(ofxRulr::Exception("No Depth to World LUT found!"));
											}

											//Sample the area around the 2D marker for an average 3D position.
											ofVec3f avgPos = ofVec3f::zero();
											size_t numFound = 0;
											const float radiusSq = marker.radius * marker.radius;
											for (int y = marker.center.y - marker.radius; y < marker.center.y + marker.radius; ++y) {
												for (int x = marker.center.x - marker.radius; x < marker.center.x + marker.radius; ++x) {
													if (ofVec2f(x, y).squareDistance(marker.center) <= radiusSq) {
														int idx = y * height + x;
														ofVec3f candidate = ofVec3f(lut[idx * 2 + 0], lut[idx * 2 + 1], 1.0f) * depth[idx] * 0.001f;
														if (candidate != ofVec3f::zero()) {
															avgPos += candidate;
															++numFound;
														}
													}
												}
											}
											if (numFound) {
												//The position is valid.

												//Set the marker position to the average.
												avgPos /= numFound;
												marker.position = avgPos;

												//Save the data.
												if (this->dataToSolve.find(it.first) == this->dataToSolve.end()) {
													map<size_t, Marker> frameToMarker;
													frameToMarker.emplace(frameNum, marker);
													this->dataToSolve.emplace(it.first, frameToMarker);
												}
												else if (this->dataToSolve[it.first].find(frameNum) == this->dataToSolve[it.first].end()) {
													this->dataToSolve[it.first].emplace(frameNum, marker);
												}
												else {
													this->dataToSolve[it.first][frameNum] = marker;
												}
												ofLogVerbose("Calibrate") << "Frame " << frameNum << ": Subscriber " << it.first << " found marker " << marker.center << " => " << marker.position << endl;
											}
											else {
												//The position is invalid, ignore it.
												marker.position = ofVec3f::zero();
											}
										}
									}
								//}
							}
						}
					}
				}

				//----------
				vector<Calibrate::Marker> Calibrate::findMarkersInFrame(const ofxMultiTrack::Frame & frame) {
					const auto & infrared = frame.getInfrared();
					this->infrared.loadData(infrared);

					auto infraRed16 = infrared; //copy local
					auto infraRed16Mat = ofxCv::toCv(infraRed16);

					auto infraRed8Mat = infraRed16Mat.clone();
					infraRed8Mat.convertTo(infraRed8Mat, CV_8U, 1.0f / float(1 << 8));

					cv::threshold(infraRed8Mat, infraRed8Mat, this->parameters.findMarker.threshold, 255, cv::THRESH_TOZERO);
					ofxCv::copy(infraRed8Mat, this->threshold);
					this->threshold.update();

					vector<vector<cv::Point2i>> contours;
					cv::findContours(infraRed8Mat, contours, CV_RETR_EXTERNAL, CV_CHAIN_APPROX_SIMPLE);

					vector<Marker> markers;
					for (auto & contour : contours) {
						if (cv::contourArea(contour) >= this->parameters.findMarker.minimumArea) {
							Marker marker;
							cv::minEnclosingCircle(contour, ofxCv::toCv(marker.center), marker.radius);
							markers.push_back(marker);
						}
					}

					return markers;
				}

				//----------
				void Calibrate::setupSolveSets() {
					if (!this->solveSets.empty()) return;

					//Go through subscribers in pairs.
					for (auto & srcIt = this->dataToSolve.begin(); srcIt != this->dataToSolve.end(); ++srcIt) {
						//Make sure there is a next element to work with.
						auto & dstIt = next(srcIt);
						if (dstIt != this->dataToSolve.end()) {
							vector<ofVec3f> srcPoints;
							vector<ofVec3f> dstPoints;
							
							auto & srcFrames = srcIt->second;
							auto & dstFrames = dstIt->second;
							for (auto & kIt : srcFrames) {
								auto frameNum = kIt.first;
								//Make sure the frame numbers match.
								if (dstFrames.find(frameNum) != dstFrames.end()) {
									//Add the points.
									srcPoints.push_back(srcFrames[frameNum].position);
									dstPoints.push_back(dstFrames[frameNum].position);
								}
							}

							if (srcPoints.size()) {
								// Set up the solver.
								auto & solver = this->solveSets[dstIt->first];
								solver.setup(srcPoints, dstPoints);
							}
						}
					}
				}

				//----------
				void Calibrate::triggerSolvers() {
					//Run all the solver threads!
					for (auto & it : this->solveSets) {
						it.second.trySolve();
					}
				}

				//----------
				void Calibrate::applyTransforms() {
					//Apply the transforms in sequence.
					ofMatrix4x4 currTransform;
					bool firstFound = false;
					auto & subscribers = this->getInput<World>()->getSubscribers();
					for (auto & it : subscribers) {
						auto weak_subscriber = it.second;
						if (!weak_subscriber.expired()) {
							auto subscriberNode = weak_subscriber.lock();
							auto subscriber = subscriberNode->getSubscriber();
							if (subscriber) {
								if (!firstFound) {
									currTransform = subscriberNode->getTransform();
									firstFound = true;
								}
								else {
									auto & result = this->solveSets[it.first].getResult();
									if (!result.success) {
										ofLogWarning("Calibrate") << "Result for Subscriber " << it.first << " was unsuccessful, use at your own risk!";
									}

									currTransform *= result.transform;
									subscriberNode->setTransform(currTransform);
								}
							}
						}
					}
				}

				//----------
				chrono::system_clock::duration Calibrate::getTimeSinceCaptureStarted() const {
					return chrono::system_clock::now() - this->captureStartTime;
				}

				//----------
				void Calibrate::serialize(Json::Value & json) {
					{
						auto & jsonToPreview = json["dataToPreview"];
						for (const auto & it : this->dataToPreview) {
							auto const key = ofToString(it.first);
							auto & jsonSubscriber = jsonToPreview[key];
							int index = 0;
							for (const auto & marker : it.second) {
								auto & jsonMarker = jsonSubscriber[index++];
								jsonMarker["center"] << marker.center;
								jsonMarker["radius"] << marker.radius;
							}
						}
					}
					{
						auto & jsonToSolve = json["dataToSolve"];
						for (const auto & it : this->dataToSolve) {
							auto const subscriberKey = ofToString(it.first);
							auto & jsonSubscriber = jsonToSolve[subscriberKey];
							for (const auto & jt : it.second) {
								auto const frameKey = ofToString(jt.first);
								auto & jsonFrame = jsonSubscriber[frameKey];
								const auto & marker = jt.second;
								jsonFrame["center"] << marker.center;
								jsonFrame["radius"] << marker.radius;
								jsonFrame["position"] << marker.position;
							}
						}
					}
					{
						auto & jsonSolveSets = json["solveSets"];
						for (auto & it : this->solveSets) {
							auto const subscriberKey = ofToString(it.first);
							auto & jsonSubscriber = jsonSolveSets[subscriberKey];
							it.second.serialize(jsonSubscriber);
						}
					}

					ofxRulr::Utils::Serializable::serialize(json["parameters"], this->parameters);
				}

				//----------
				void Calibrate::deserialize(const Json::Value & json) {
					{
						this->dataToPreview.clear();
						const auto & jsonToPreview = json["dataToPreview"];
						const auto subscriberKeys = jsonToPreview.getMemberNames();
						for (const auto & subscriberKey : subscriberKeys) {
							vector<Marker> markers;
							for (const auto & jsonMarker : jsonToPreview[subscriberKey]) {
								Marker marker;
								jsonMarker["center"] >> marker.center;
								jsonMarker["radius"] >> marker.radius;
								markers.push_back(marker);
							}
							size_t subscriberFirst = ofToInt(subscriberKey);
							this->dataToPreview.emplace(subscriberFirst, markers);
						}
					}
					{
						this->dataToSolve.clear();
						const auto & jsonToSolve = json["dataToSolve"];
						const auto subscriberKeys = jsonToSolve.getMemberNames();
						for (const auto & subscriberKey : subscriberKeys) {
							map<size_t, Marker> markers;
							const auto & jsonSubscriber = jsonToSolve[subscriberKey];
							const auto frameKeys = jsonSubscriber.getMemberNames();
							for (const auto & frameKey : frameKeys) {
								auto & jsonMarker = jsonSubscriber[frameKey];
								Marker marker;
								jsonMarker["center"] >> marker.center;
								jsonMarker["radius"] >> marker.radius;
								jsonMarker["position"] >> marker.position;

								size_t frameFirst = ofToInt(frameKey);
								markers.emplace(frameFirst, marker);
							}
							size_t subscriberFirst = ofToInt(subscriberKey);
							this->dataToSolve.emplace(subscriberFirst, markers);
						}
					}
					{
						this->solveSets.clear();
						const auto & jsonSolveSets = json["solveSets"];
						const auto subscriberKeys = jsonSolveSets.getMemberNames();
						for (const auto & subscriberKey : subscriberKeys) {
							const auto & jsonSubscriber = jsonSolveSets[subscriberKey];
							ofxRulr::Utils::SolveSet solveSet;
							solveSet.deserialize(jsonSubscriber);
							size_t subscriberFirst = ofToInt(subscriberKey);
							this->solveSets.emplace(subscriberFirst, solveSet);
						}
					}

					ofxRulr::Utils::Serializable::deserialize(json["parameters"], this->parameters);
				}

				//----------
				const ofColor & Calibrate::getSubscriberColor(size_t key) {
					if (this->subscriberColors.find(key) == this->subscriberColors.end()) {
						ofColor color(200, 100, 100);
						color.setHueAngle((key * 50) % 360);
						this->subscriberColors.emplace(key, color);
					}
					return this->subscriberColors[key];
				}
			}
		}
	}
}

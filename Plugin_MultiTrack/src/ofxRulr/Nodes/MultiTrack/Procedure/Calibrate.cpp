#include "pch_MultiTrack.h"
#include "Calibrate.h"

#include "../Utils.h"

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

					this->dialogStep = DialogStepClosed;

					this->panel = ofxCvGui::Panels::makeWidgets();
					auto button = this->panel->addButton("Open Capture", [this]() {
						this->dialogStepTo(DialogStepBegin);
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
					if (this->dialogStep == DialogStepCapture) {
						this->captureFrame(true);

						if (this->getTimeSinceCaptureStarted() > chrono::seconds(this->parameters.capture.duration)) {
							this->dialogStepTo(DialogStepComplete);
						}
					}
					else if (this->dialogStep == DialogStepBegin || this->parameters.debugWorld.liveMarker) {
						this->captureFrame(false);
					}
				}

				//----------
				void Calibrate::drawWorld() {
					if (!(this->parameters.debugWorld.drawLines || this->parameters.debugWorld.drawPoints || this->parameters.debugWorld.liveMarker)) {
						return;
					}

					auto world = this->getInput<World>();
					if (!world) return;

					//Cache the transforms and debug colors for each subscriber.
					map<size_t, ofMatrix4x4> transforms;
					map<size_t, ofFloatColor> debugColors;
					auto & subscribers = world->getSubscribers();
					for (auto & it : subscribers) {
						auto weak_subscriber = it.second;
						if (!weak_subscriber.expired()) {
							auto subscriberNode = weak_subscriber.lock();
							transforms.emplace(it.first, subscriberNode->getTransform());
							debugColors.emplace(it.first, subscriberNode->getSubscriberColor());
						}
					}

					//Draw lines between matching points of different Subscribers
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

										colors.push_back(debugColors[srcIt->first]);
										colors.push_back(debugColors[dstIt->first]);
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

					//Draw the point data for each subscriber.
					if (this->parameters.debugWorld.drawPoints) {
						for (auto & it = this->dataToSolve.begin(); it != this->dataToSolve.end(); ++it) {
							if (transforms.find(it->first) == transforms.end()) break;
							const auto & transform = transforms[it->first];

							ofMesh mesh;
							for (const auto & jt : this->dataToSolve[it->first]) {
								mesh.addVertex(jt.second.position);
							}

							ofPushMatrix();
							ofPushStyle();
							{
								ofSetColor(debugColors[it->first]);
								ofMultMatrix(transform);

								mesh.draw(OF_MESH_POINTS);
							}
							ofPopStyle();
							ofPopMatrix();
						}
					}

					//Draw the live marker(s) for each subscriber.
					if (this->parameters.debugWorld.liveMarker) {
						for (auto & it = this->dataToPreview.begin(); it != this->dataToPreview.end(); ++it) {
							if (transforms.find(it->first) == transforms.end()) break;
							const auto & transform = transforms[it->first];
							const auto & origin = ofVec3f::zero() * transform;

							ofPushMatrix();
							ofPushStyle();
							{
								ofSetColor(debugColors[it->first]);
								for (const Marker & marker : it->second) {
									ofNode node;
									node.setPosition(marker.position * transform);
									node.lookAt(origin);

									ofVec3f axis;
									float angle;
									node.getOrientationQuat().getRotate(angle, axis);

									//Translate the Marker to its position.
									ofTranslate(node.getPosition());

									//Perform the rotation (billboard).
									ofRotate(angle, axis.x, axis.y, axis.z);

									marker.valid ? ofFill() : ofNoFill();
									ofDrawCircle(0, 0, 0.1f);
								}
							}
							ofPopStyle();
							ofPopMatrix();
						}
					}

				}

				//----------
				void Calibrate::dialogStepTo(DialogStep nextStep) {
					// TODO: Turn this back on once we get a close callback on the Dialog
					//if (this->currStep == nextStep) return;

					this->dialogStep = nextStep;

					ofxCvGui::closeDialog();
					this->dialog.reset();

					if (this->dialogStep == DialogStepClosed) {
						ofxCvGui::refreshInspector(this);
						return;
					}

					auto addPreviews = [this](shared_ptr<ofxCvGui::Panels::Widgets> & panel, bool history) {
						auto strip = ofxCvGui::Panels::Groups::makeStrip();
						strip->setHeight(360.0f);
						panel->add(strip);

						auto & subscribers = this->getInput<World>()->getSubscribers();
						for (auto & it : subscribers) {
							auto weak_subscriber = it.second;
							if (!weak_subscriber.expired()) {
								auto subscriber = weak_subscriber.lock();

								auto texture = ofxCvGui::Panels::makeTexture(subscriber->getDepthTexture());
								texture->setWidth(480.0f);
								texture->setHeight(320.0f);
								strip->add(texture);

								auto key = it.first;
								auto color = subscriber->getSubscriberColor();
								texture->onDrawImage += [this, key, color, history](ofxCvGui::DrawImageArguments & args) {
									ofPushStyle();
									{
										ofSetColor(color);
										
										if (history) {
											//Draw captured previous frame markers.
											const auto & frames = this->dataToSolve[key];
											if (frames.size()) {
												const auto lastFrame = (*frames.rbegin()).first;
												for (const auto & it : frames) {
													ofDrawCircle(it.second.center, MAX(it.second.radius - lastFrame + it.first, it.second.radius * 0.25f));
												}
											}
										}
										{
											//Draw live current frame markers.
											ofFill();
											const auto & markers = this->dataToPreview[key];
											for (const auto & m : markers) {
												m.valid ? ofFill() : ofNoFill();
												ofDrawCircle(m.center, m.radius);
											}
										}
									}
									ofPopStyle();
								};
							}
						}
					};

					shared_ptr<ofxCvGui::Panels::Widgets> stepPanel;
					stepPanel = ofxCvGui::Panels::makeWidgets();
					stepPanel->addTitle("MultiTrack");
					
					if (this->dialogStep == DialogStepBegin) {
						//GUI.
						stepPanel->addTitle("Step 1: Get Ready!", ofxCvGui::Widgets::Title::Level::H2);
						stepPanel->addTitle("Grab your marker and make sure it's the only red thing in the previews below. Adjust the parameters if necessary.", ofxCvGui::Widgets::Title::Level::H3);

						stepPanel->addParameterGroup(parameters.capture);
						stepPanel->addParameterGroup(parameters.findMarker);

						addPreviews(stepPanel, false);

						auto buttonNext = stepPanel->addButton("Begin Capture", [this]() {
							this->dialogStepTo(DialogStepCapture);
						});
						buttonNext->setHeight(100.0f);
					}
					else if (this->dialogStep == DialogStepCapture) {
						//Clear previous data.
						this->dataToSolve.clear();
						this->solveSets.clear();

						//Start the timer.
						this->captureStartTime = chrono::system_clock::now(); 
						
						//GUI.
						stepPanel->addTitle("Step 2: Capture", ofxCvGui::Widgets::Title::Level::H2);
						stepPanel->addTitle("Stop reading this and walk across the entire sensor array, making sure the marker is visible.", ofxCvGui::Widgets::Title::Level::H3);

						stepPanel->addLiveValue<float>("Capture remaining [s]", [this]() {
							// Using * 1000.0f so we get floats.
							return (this->parameters.capture.duration * 1000.0f - chrono::duration_cast<chrono::milliseconds>(this->getTimeSinceCaptureStarted()).count()) / 1000.0f;
						});

						addPreviews(stepPanel, true);

						auto buttonNext = stepPanel->addButton("Continue", [this]() {
							this->dialogStepTo(DialogStepComplete);
						});
						buttonNext->setHeight(100.0f);
					}
					else if (this->dialogStep == DialogStepComplete) {
						this->setupSolveSets();

						//GUI.
						stepPanel->addTitle("Step 3: Complete", ofxCvGui::Widgets::Title::Level::H2);
						stepPanel->addTitle("You're done here. Close this dialog and move on to the Inspector to finish calibration.", ofxCvGui::Widgets::Title::Level::H3);

						addPreviews(stepPanel, true);

						auto buttonNext = stepPanel->addButton("Close Dialog", [this]() {
							this->dialogStepTo(DialogStepClosed);
						});
						buttonNext->setHeight(100.0f);
					}

					//Build the Dialog
					auto strip = ofxCvGui::Panels::Groups::makeStrip();
					strip->setDirection(ofxCvGui::Panels::Groups::Strip::Horizontal);

					//Add the common panel.
					{
						auto commonPanel = ofxCvGui::Panels::makeWidgets();

						auto buttonClose = commonPanel->addButton("Close", [this]() {
							this->dialogStepTo(DialogStepClosed);
						});
						buttonClose->setHeight(50.0f);

						strip->add(commonPanel);
					}

					//Add the step panel.
					strip->add(stepPanel);

					strip->setCellSizes({ 100, -1 });
					this->dialog = strip;

					//Make sure the dialog is open.
					ofxCvGui::openDialog(this->dialog);
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
							this->setupSolveSets();
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

					inspector->addParameterGroup(parameters.debugWorld);
				}

				//----------
				void Calibrate::captureFrame(bool record) {
					auto world = this->getInput<World>();
					if (!world) return;

					size_t frameNum = ofGetFrameNum();
					Marker marker;

					//Get the marker data for this frame.
					auto & subscribers = world->getSubscribers();
					for (auto & it : subscribers) {
						auto weak_subscriber = it.second;
						if (!weak_subscriber.expired()) {
							auto subscriberNode = weak_subscriber.lock();
							auto subscriber = subscriberNode->getSubscriber();
							if (subscriber) {
								//Make sure all the data is ready before proceeding.
								auto width = subscriber->getFrame().getDepth().getWidth();
								if (width == 0) continue;
								 
								auto height = subscriber->getFrame().getDepth().getHeight();
								if (height == 0) continue;

								auto depth = subscriber->getFrame().getDepth().getData();
								if (depth == nullptr) continue;

								auto lut = subscriberNode->getDepthToWorldLUT().getData();
								if (lut == nullptr)  continue;

								//if (subscriber->isFrameNew()) {
									//Find the Markers in the frame.
									vector<Marker> markers = findMarkersInFrame(subscriber->getFrame());

									for (auto & marker : markers) {
										this->mapMarkerToWorld(marker, width, height, depth, lut);
									}

									//Keep all found Markers for preview.
									if (this->dataToPreview.find(it.first) == this->dataToPreview.end()) {
										this->dataToPreview.emplace(it.first, markers);
									}
									else {
										this->dataToPreview[it.first] = markers;
									}

									if (record) {
										if (markers.size() == 1) {
											auto & marker = markers.front();

											if (marker.valid) {
												//Save the data for solving.
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
				bool Calibrate::mapMarkerToWorld(Marker & marker, int frameWidth, int frameHeight, const unsigned short * depthData, const float * lut) {
					//Sample the area around the 2D marker for an average 3D position.
					ofVec3f avgPos = ofVec3f::zero();
					size_t numFound = 0;
					
					const float radiusSq = marker.radius * marker.radius;
					
					int minY = MAX(0, marker.center.y - marker.radius);
					int maxY = MIN(marker.center.y + marker.radius, frameHeight - 1);
					int minX = MAX(0, marker.center.x - marker.radius);
					int maxX = MIN(marker.center.x + marker.radius, frameWidth - 1);

					for (int y = minY; y < maxY; ++y) {
						for (int x = minX; x < maxX; ++x) {

							if (ofVec2f(x, y).squareDistance(marker.center) <= radiusSq) {
								int idx = y * frameWidth + x;
								ofVec3f candidate = ofVec3f(lut[idx * 2 + 0], lut[idx * 2 + 1], 1.0f) * depthData[idx] * 0.001f;
								if (candidate != ofVec3f::zero()) {
									//Valid mapping.
									avgPos += candidate;
									++numFound;
								}
							}
							//break;
						}
						//break;
					}
					if (numFound) {
						//The position is valid, set the marker position to the average.
						avgPos /= numFound;
						marker.position = avgPos;

						const auto dist = marker.position.length();
						if (this->parameters.findMarker.clipNear < dist && dist < this->parameters.findMarker.clipFar) {
							//Valid range.
							marker.valid = true;
						}
						else {
							marker.valid = false;
						}
						
					}
					else {
						//The position is invalid, ignore it.
						marker.position = ofVec3f::zero();
						marker.valid = false;
					}
					
					return marker.valid;
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
							if (!firstFound) {
								currTransform = subscriberNode->getTransform();
								firstFound = true;
							}
							else {
								auto & result = this->solveSets[it.first].getResult();
								if (!result.success) {
									ofLogWarning("Calibrate") << "Result for Subscriber " << it.first << " was unsuccessful, use at your own risk!";
								}

								currTransform *= result.transform.getInverse();
								subscriberNode->setTransform(currTransform);
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
			}
		}
	}
}

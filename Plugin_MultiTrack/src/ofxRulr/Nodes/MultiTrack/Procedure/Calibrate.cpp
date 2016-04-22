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
					//RULR_NODE_INSPECTOR_LISTENER;

					this->currStep = StepIdle;

					this->panel = ofxCvGui::Panels::makeWidgets();
					auto button = this->panel->addButton("Open Dialogue", [this]() {
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
						this->captureFrame(CapturePreview);
					}
					else if (this->currStep == StepCapture) {
						this->captureFrame(CaptureSolve);

						if (this->getTimeSinceCaptureStarted() > chrono::seconds(this->parameters.capture.duration)) {
							this->goToStep(StepSolve);
						}
					}
					else if (this->currStep == StepSolve) {
						if (this->solveSets.size() > 0) {
							ofxRulr::Utils::ScopedProcess scopedProcess("Solving");
							try {
								size_t numCompleted = 0;
								for (auto & it : this->solveSets) {
									if (it.second.didComplete()) {
										++numCompleted;
									}
									auto result = it.second.getResult();
									cout << "  " << it.first << ": ";
									if (it.second.didComplete()) {
										cout << " Finished in " << chrono::duration_cast<chrono::seconds>(result.totalTime).count() << " seconds with residual " << result.residual << endl;
									}
									else {
										cout << " Running for " << chrono::duration_cast<chrono::seconds>(it.second.getRunningTime()).count() << " seconds with residual " << result.residual << endl;
									}
								}
								if (numCompleted == this->solveSets.size()) {
									scopedProcess.end();
								}
							}
							RULR_CATCH_ALL_TO_ALERT
						}
					}
				}

				//----------
				void Calibrate::goToStep(Step nextStep) {
					// TODO: Turn this back on once we get a close callback on the Dialogue
					//if (this->currStep == nextStep) return;

					this->currStep = nextStep;

					ofxCvGui::closeDialogue();
					this->dialogue.reset();

					shared_ptr<ofxCvGui::Panels::Widgets> panel;
					if (this->currStep == StepBegin) {
						panel = ofxCvGui::Panels::makeWidgets();
						panel->addTitle("MultiTrack Calibration");
						panel->addLiveValue<string>("Instructions", [this]() {
							return "Pick up your marker and do the thing.";
						});
						panel->addParameterGroup(parameters.capture);
						panel->addParameterGroup(parameters.findMarker);

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
								texture->onDrawImage += [this, key](ofxCvGui::DrawImageArguments & args) {
									ofPushStyle();
									ofSetColor(255, 0, 0, 127);
									{
										auto & markers = this->dataToPreview[key];
										for (Marker & m : markers) {
											ofDrawCircle(m.center, m.radius);
										}
									}
									ofPopStyle();
								};

								++count;
							}
						}

						auto buttonNext = panel->addButton("Begin Capture", [this]() {
							this->captureStartTime = chrono::system_clock::now();
							this->goToStep(StepCapture);
						});
						buttonNext->setHeight(100.0f);
					}
					else if (this->currStep == StepCapture) {
						//Clear previous data.
						this->dataToSolve.clear();

						panel = ofxCvGui::Panels::makeWidgets();
						panel->addTitle("MultiTrack Capture");

						auto buttonBack = panel->addButton("Restart", [this]() {
							this->goToStep(StepBegin);
						});

						panel->addLiveValue<float>("Capture remaining [s]", [this]() {
							// Using * 1000.0f so we get floats.
							return (this->parameters.capture.duration * 1000.0f - chrono::duration_cast<chrono::milliseconds>(this->getTimeSinceCaptureStarted()).count()) / 1000.0f;
						});

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

								++count;
							}
						}

						auto buttonNext = panel->addButton("Solve", [this]() {
							this->goToStep(StepSolve);
						});
						buttonNext->setHeight(100.0f);
					}
					else if (this->currStep == StepSolve) {
						this->setupSolveSets();
						
						panel = ofxCvGui::Panels::makeWidgets();
						panel->addTitle("MultiTrack Solve");

						for (auto & it : this->solveSets) {
							//auto & solveSet = it.second;
							//auto & result = solveSet.getResult();

							//panel->addTitle("Solver " + ofToString(it.first), ofxCvGui::Widgets::Title::Level::H2);

							//bool running = solveSet.isRunning();
							//panel->addIndicator("Running", [this, &solveSet]() {

							//	if (this->subscriber) {
							//		if (this->subscriber->getSubscriber().isFrameNew()) {
							//			return Widgets::Indicator::Status::Good;
							//		}
							//	}
							//	return Widgets::Indicator::Status::Clear;
							//});
							//panel->addLiveValue("Residual", [this, result]() {
							//	return result.residual;
							//});

						}

						auto buttonBack = panel->addButton("Restart", [this]() {
							this->goToStep(StepBegin);
						});

						panel->addParameterGroup(this->parameters.solveTransform);

						auto buttonConfirm = panel->addButton("Confirm", [this]() {
							this->goToStep(StepConfirm);
						});
						buttonConfirm->setHeight(100.0f);

						//Get crackin'
						try {
							this->triggerSolvers();
						}
						RULR_CATCH_ALL_TO_ALERT
					}
					else if (this->currStep == StepConfirm) {
						auto buttonNext = panel->addButton("Apply", [this]() {
							this->applyTransforms();
							this->goToStep(StepIdle);
						});
						buttonNext->setHeight(100.0f);
					}

					if (panel) {
						//Build the dialogue
						auto strip = ofxCvGui::Panels::Groups::makeStrip();
						strip->setDirection(ofxCvGui::Panels::Groups::Strip::Horizontal);

						//Add the common panel.
						{
							auto commonPanel = ofxCvGui::Panels::makeWidgets();
							auto button = commonPanel->addButton("Close", [this]() {
								this->goToStep(StepIdle);
							});
							button->setHeight(50.0f);
							strip->add(commonPanel);
						}

						//Add the current panel.
						strip->add(panel);

						strip->setCellSizes({ 100, -1 });
						this->dialogue = strip;

						//Make sure the dialog is open.
						ofxCvGui::openDialogue(this->dialogue);
					}
				}

				//----------
				void Calibrate::captureFrame(CaptureMode mode) {
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
								if (subscriber->isFrameNew()) {
									//Find the Markers in the frame.
									vector<Marker> markers = findMarkersInFrame(subscriber->getFrame());

									if (mode == CapturePreview) {
										//Keep all found Markers, don't bother mapping to 3D.
										if (this->dataToPreview.find(it.first) == this->dataToPreview.end()) {
											this->dataToPreview.emplace(it.first, markers);
										}
										else {
											this->dataToPreview[it.first] = markers;
										}
									}
									else {
										if (markers.size() == 1) {
											auto & marker = markers.front();

											//Map the Marker coordinates to world space.
											auto height = subscriber->getFrame().getDepth().getHeight();
											auto depth = subscriber->getFrame().getDepth().getData();
											auto lut = subscriberNode->getDepthToWorldLUT().getData();
											if (lut == nullptr) {
												ofLogError("Calibrate") << "No Depth to World LUT found for Subscriber " << it.first;
												throw;
											}
											int idx = marker.center.y * height + marker.center.x;
											marker.position = ofVec3f(lut[idx * 2 + 0], lut[idx * 2 + 1], 1.0f) * depth[idx] * 0.001f;

											//If the position is valid, save the data.
											if (marker.position != ofVec3f::zero()) {
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
								}
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
					//Clear previous solvers.
					this->solveSets.clear();

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
									auto result = this->solveSets[it.first].getResult();
									if (result.success) {
										currTransform *= result.transform;
										subscriberNode->setTransform(currTransform);
									}
									else {
										ofLogWarning("Calibrate") << "Skipping unsuccessful result for Subscriber " << it.first;
									}
								}
							}
						}
					}
				}

				//----------
				void Calibrate::populateInspector(ofxCvGui::InspectArguments & inspectArgs) {
					
				}

				//----------
				chrono::system_clock::duration Calibrate::getTimeSinceCaptureStarted() const {
					return chrono::system_clock::now() - this->captureStartTime;
				}
			}
		}
	}
}
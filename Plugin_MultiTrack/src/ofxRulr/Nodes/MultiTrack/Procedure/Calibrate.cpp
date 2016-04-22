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
					if (this->currStep == StepCapture) {
						this->addCapture();

						if (this->getTimeSinceCaptureStarted() > chrono::seconds(this->parameters.capture.duration)) {
							this->goToStep(StepSolve);
						}
					}
					else if (this->currStep == StepSolve) {
						if (this->solveSets.size() > 0) {
							cout << "Solving..." << endl;
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
								//All done!
								cout << "All done!" << endl;
							}
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

						auto buttonNext = panel->addButton("Begin Capture", [this]() {
							this->captureStartTime = chrono::system_clock::now();
							this->goToStep(StepCapture);
						});
						buttonNext->setHeight(100.0f);
					}
					else if (this->currStep == StepCapture) {
						//Clear previous data.
						this->dataToEvaluate.clear();

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
							void applyTransforms();
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
				void Calibrate::addCapture() {
					size_t frameNum = ofGetFrameNum();
					cout << "Add Capture " << frameNum << endl;

					//Get the marker data for this frame.
					auto & subscribers = this->getInput<World>()->getSubscribers();
					for (auto & it : subscribers) {
						auto weak_subscriber = it.second;
						if (!weak_subscriber.expired()) {
							auto node = weak_subscriber.lock();
							auto subscriber = node->getSubscriber();
							if (subscriber) {
								if (subscriber->isFrameNew()) {
									//Find the Markers in the frame.
									vector<Marker> markers;
									findMarkerInFrame(subscriber->getFrame(), markers);

									//Map the Marker coordinates to world space.
									auto height = subscriber->getFrame().getDepth().getHeight();
									auto depth = subscriber->getFrame().getDepth().getData();
									auto lut = node->getDepthToWorldLUT().getData();
									for (auto & m : markers) {
										int idx = m.center.y * height + m.center.x;
										m.position = ofVec3f(lut[idx * 2 + 0], lut[idx * 2 + 1], 1.0f) * depth[idx] * 0.001f;
									}

									//Save the data.
									auto & subscriberName = node->getName() + "_" + ofToString(it.first);
									this->dataToEvaluate[it.first][frameNum] = markers;

									cout << "  Subscriber " << subscriberName << " has " << markers.size() << " markers" << endl;
									for (int i = 0; i < markers.size(); ++i) {
										cout << "    " << i << ": " << markers[i].center << " => " << markers[i].position << endl;
									}
								}
							}
						}
					}
				}

				//----------
				void Calibrate::findMarkerInFrame(const ofxMultiTrack::Frame & frame, vector<Marker> & markers) {
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

					markers.clear();
					for (auto & contour : contours) {
						if (cv::contourArea(contour) >= this->parameters.findMarker.minimumArea) {
							Marker marker;
							cv::minEnclosingCircle(contour, ofxCv::toCv(marker.center), marker.radius);
							markers.push_back(marker);
						}
					}
				}

				//----------
				void Calibrate::setupSolveSets() {
					//Clear previous solvers.
					this->solveSets.clear();

					//Go through subscribers in pairs.
					for (auto & srcIt = this->dataToEvaluate.begin(); srcIt != this->dataToEvaluate.end(); ++srcIt) {
						//Make sure there is a next element to work with.
						auto & dstIt = next(srcIt);
						if (dstIt != this->dataToEvaluate.end()) {
							auto & srcData = srcIt->second;
							auto & dstData = dstIt->second;
							for (auto & kt : srcData) {
								auto frameNum = kt.first;
								//Make sure the frame numbers match.
								if (dstData.find(frameNum) != dstData.end()) {
									auto & srcMarkers = kt.second;
									auto & dstMarkers = dstData[frameNum];
									if (srcMarkers.size() == dstMarkers.size()) {
										//Everything matches!

										//Add the points.
										vector<ofVec3f> srcPoints;
										vector<ofVec3f> dstPoints;
										for (int i = 0; i < srcMarkers.size(); ++i) {
											srcPoints.push_back(srcMarkers[i].position);
											dstPoints.push_back(dstMarkers[i].position);
										}

										// Set up the solver.
										this->solveSets[dstIt->first].setup(srcPoints, dstPoints);
									}
								}
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
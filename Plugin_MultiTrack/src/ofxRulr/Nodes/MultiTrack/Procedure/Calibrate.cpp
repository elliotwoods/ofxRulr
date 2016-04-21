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
						this->markerData.clear();

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

								auto pixels = ofxCvGui::Panels::makePixels(subscriber->getSubscriber()->getFrame().getDepth());
								pixels->setHeight(360.0f);
								strip->add(pixels);

								++count;
							}
						}

						auto buttonNext = panel->addButton("Solve", [this]() {
							this->goToStep(StepSolve);
						});
						buttonNext->setHeight(100.0f);
					}
					else if (this->currStep == StepSolve) {
						panel = ofxCvGui::Panels::makeWidgets();
						panel->addTitle("MultiTrack Solve");

						auto buttonBack = panel->addButton("Restart", [this]() {
							this->goToStep(StepBegin);
						});

						panel->addParameterGroup(this->parameters.solveTransform);

						auto buttonSolve = panel->addButton("Solve", [this]() {
							try {
								this->solveAll();
							}
							RULR_CATCH_ALL_TO_ALERT
						}, OF_KEY_RETURN);
						buttonSolve->setHeight(100.0f);
					}
					//else if (this->currStep == StepConfirm) {
					//	inspector->add(MAKE(ofxCvGui::Widgets::LiveValue<float>, "Reprojection error [px]", [this]() {
					//		return this->error;
					//	}));

					//	auto buttonNext = panel->addButton("Solve", [this]() {
					//		this->goToStep(StepConfirm);
					//	});
					//	buttonNext->setHeight(100.0f);
					//}

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

					// Get the marker data for this frame.
					auto & subscribers = this->getInput<World>()->getSubscribers();
					map<string, vector<Marker>> subscriberData;
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
									auto & name = node->getName();
									subscriberData[name] = markers;
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
				void Calibrate::solveAll() {

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
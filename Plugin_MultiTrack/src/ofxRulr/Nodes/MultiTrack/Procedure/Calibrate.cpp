#include "pch_MultiTrack.h"
#include "Calibrate.h"

#include "ofxRulr/Utils/ScopedProcess.h"

#include "ofxCvGui/Widgets/Button.h"

#include "ofxRulr/Nodes/MultiTrack/World.h"

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

					this->currStep = StepIdle;

					this->addInput<ofxRulr::Nodes::MultiTrack::World>();

					//Build the step panels.
					this->stepPanels.resize(NumSteps);
					{
						this->stepPanels[StepIdle] = nullptr;
					}
					{
						this->stepPanels[StepBegin] = ofxCvGui::Panels::makeWidgets();;
						auto & panelBegin = this->stepPanels[StepBegin];
						panelBegin->addTitle("MultiTrack Calibration");
						panelBegin->addLiveValue<string>("Instructions", [this]() {
							return "Pick up your marker and do the thing.";
						});
						panelBegin->addParameterGroup(parameters.capture);
						panelBegin->addParameterGroup(parameters.findMarker);
						auto button = panelBegin->addButton("Begin Capture", [this]() {
							this->captureStartTime = chrono::system_clock::now();
							this->goToStep(StepIdle);
						});
						button->setHeight(100.0f);
					}

					//Build the node and setup the dialogue.
					this->panel = ofxCvGui::Panels::makeWidgets();
					this->panel->addButton("Open Dialogue", [this]() {
						this->goToStep(StepBegin);
					});
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

					if (this->dialogue) {
						this->dialogue->clear();
					}

					auto & currPanel = this->stepPanels[this->currStep];
					if (currPanel) {
						if (!this->dialogue) {
							//Build the dialogue
							this->dialogue = make_shared<ofxCvGui::Panels::Widgets>();
							this->dialogue->addTitle("Dialogue");
							this->dialogue->addButton("Close", [this]() {
								this->goToStep(StepIdle);
							});
						}

						//Add the current panel.
						this->dialogue->add(currPanel);
						
						//Make sure the dialog is open.
						if (!ofxCvGui::isDialogueOpen()) {
							ofxCvGui::openDialogue(this->dialogue);
						}
					}
					else {
						//Close and delete the dialog.
						ofxCvGui::closeDialogue();
						this->dialogue.reset();
					}
				}

				//----------
				void Calibrate::addCapture() {

				}

				//----------
				void Calibrate::solveAll() {

				}

				//----------
				void Calibrate::populateInspector(ofxCvGui::InspectArguments & inspectArgs) {
					auto inspector = inspectArgs.inspector;

					inspector->addParameterGroup(this->parameters.capture);
					//this->parameters.capture.enabled.addListener(this, &Calibrate::captureToggled);

					//inspector->addLiveValue<float>("Capture remaining [s]", [this]() {
					//	if (this->parameters.capture.enabled) {
					//		// Using * 1000.0f so we get floats.
					//		return (this->parameters.capture.duration * 1000.0f - chrono::duration_cast<chrono::milliseconds>(this->getTimeSinceCaptureStarted()).count()) / 1000.0f;
					//	}
					//	else {
					//		return 0.0f;
					//	}
					//});

					inspector->add(MAKE(ofxCvGui::Widgets::Button, "Clear capture data", [this]() {
						this->markerData.clear();
					}));

					inspector->addParameterGroup(this->parameters.findMarker);

					inspector->addParameterGroup(this->parameters.solveTransform);

					auto solveButton = MAKE(ofxCvGui::Widgets::Button, "Solve", [this]() {
						try {
							this->solveAll();
						}
						RULR_CATCH_ALL_TO_ALERT
					}, OF_KEY_RETURN);
					solveButton->setHeight(100.0f);
					inspector->add(solveButton);
					inspector->add(MAKE(ofxCvGui::Widgets::LiveValue<float>, "Reprojection error [px]", [this]() {
						return this->error;
					}));
				}

				//----------
				chrono::system_clock::duration Calibrate::getTimeSinceCaptureStarted() const {
					return chrono::system_clock::now() - this->captureStartTime;
				}
			}
		}
	}
}
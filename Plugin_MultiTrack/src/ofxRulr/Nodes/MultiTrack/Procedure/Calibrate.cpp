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

					this->addInput<ofxRulr::Nodes::MultiTrack::World>();

					this->panel = ofxCvGui::Panels::Groups::makeStrip();
				}

				//----------
				ofxCvGui::PanelPtr Calibrate::getPanel() {
					return this->panel;
				}

				//----------
				void Calibrate::update() {
					if (this->parameters.capture.enabled) {
						this->addCapture();

						if (this->getTimeSinceCaptureStarted() > chrono::seconds(this->parameters.capture.duration)) {
							//Stop capture.
							this->parameters.capture.enabled = false;
						}
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
					this->parameters.capture.enabled.addListener(this, &Calibrate::captureToggled);

					inspector->addLiveValue<float>("Capture remaining [s]", [this]() {
						if (this->parameters.capture.enabled) {
							// Using * 1000.0f so we get floats.
							return (this->parameters.capture.duration * 1000.0f - chrono::duration_cast<chrono::milliseconds>(this->getTimeSinceCaptureStarted()).count()) / 1000.0f;
						}
						else {
							return 0.0f;
						}
					});

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
				void Calibrate::captureToggled(bool &) {
					this->captureStartTime = chrono::system_clock::now();
				}

				//----------
				chrono::system_clock::duration Calibrate::getTimeSinceCaptureStarted() const {
					return chrono::system_clock::now() - this->captureStartTime;
				}
			}
		}
	}
}
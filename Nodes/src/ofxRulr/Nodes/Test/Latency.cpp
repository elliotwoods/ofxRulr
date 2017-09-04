#include "pch_RulrNodes.h"
#include "Latency.h"

#include "ofxRulr/Nodes/Item/Camera.h"
#include "ofxRulr/Nodes/System/VideoOutput.h"

namespace ofxRulr {
	namespace Nodes {
		namespace Test {
			//----------
			Latency::Latency() {
				RULR_NODE_INIT_LISTENER;
			}

			//----------
			string Latency::getTypeName() const {
				return "Test::Latency";
			}

			//----------
			void Latency::init() {
				RULR_NODE_UPDATE_LISTENER;
				RULR_NODE_INSPECTOR_LISTENER;
				this->manageParameters(this->parameters);

				this->addInput<Item::Camera>();
				this->addInput<System::VideoOutput>();
			}

			//----------
			void Latency::update() {

			}

			//----------
			ofxCvGui::PanelPtr Latency::getPanel() {
				return this->panel;
			}

			//----------
			void Latency::populateInspector(ofxCvGui::InspectArguments & inspectArgs) {
				auto inspector = inspectArgs.inspector;

				inspector->addSpacer();

				{
					auto runTestButton = inspector->addButton("Run", [this]() {
						try {
							Utils::ScopedProcess scopedProcess("Running latency test...");
							this->run();
							scopedProcess.end();
						}
						RULR_CATCH_ALL_TO_ALERT;
					}, ' ');
					runTestButton->setHeight(100.0f);
				}
			}

			//----------
			void Latency::run() {
				this->throwIfMissingAnyConnection();

				//scan
				{
					this->scan.run(this);
				}
			}

#pragma mark Scan
			//----------
			void Latency::Scan::run(Latency * parent) {
				this->payload.init(4, 4);
				this->encoder.init(this->payload);
				this->decoder.init(this->payload);

				auto camera = parent->getInput<Item::Camera>();
				auto videoOutput = parent->getInput<System::VideoOutput>();

				if (!videoOutput->isWindowOpen()) {
					throw(Exception("VideoOutput window is not open"));
				}
				//send the first frame
				ofImage message;
				while (this->encoder >> message) {
					videoOutput->begin();
					{
						message.draw(0, 0, videoOutput->getWidth(), videoOutput->getHeight());
					}
					videoOutput->end();
					videoOutput->presentFbo();

					ofxCvGui::Utils::drawProcessingNotice("Scanning frame #" + ofToString(encoder.getFrame()));

					ofSleepMillis(1000);

					auto frame = camera->getGrabber()->getFreshFrame();
					decoder << frame->getPixels();
				}
			}
		}
	}
}
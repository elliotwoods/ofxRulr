#pragma once

#include "ofxRulr/Nodes/Base.h"
#include "ofxSpout.h"
#include "../../../addons/ofxOsc/src/ofxOsc.h"

namespace ofxRulr {
	namespace Nodes {
		namespace Item {
			namespace Orbbec {
				class DrawFeature : public Nodes::Base {
				public:
					DrawFeature();
					string getTypeName() const override;
					void init();
					void update();

					void drawFeature();

					ofxCvGui::PanelPtr getPanel() override;

				protected:
					struct : ofParameterGroup {
						ofParameter<bool> drawToWorld{ "Draw to world", false };
						struct : ofParameterGroup {
							ofParameter<bool> mesh{ "Mesh", true };
							ofParameter<bool> skeleton{ "Skeleton", true };
							
							struct : ofParameterGroup {
								ofParameter<bool> enabled{"Enabled", false};
								ofParameter<int> labelIndex{ "Label index", 6 };
								PARAM_DECLARE("Label probability", enabled, labelIndex);
							} labelProbability;

							PARAM_DECLARE("Draw", mesh, skeleton, labelProbability)
						} draw;

						struct : ofParameterGroup {
							ofParameter<bool> enabled{ "Enabled", false };
							ofParameter<string> address{ "Address", "127.0.0.1" };
							ofParameter<int> port{ "Port", 4444 };
							PARAM_DECLARE("OSC target", enabled, address, port);
						} oscTarget;

						struct : ofParameterGroup {
							ofParameter<bool> enabled{ "Enabled", false };
							ofParameter<int> port{ "Port", 4445 };
							PARAM_DECLARE("OSC receiver", enabled, port);
						} oscReceiver;

						struct : ofParameterGroup {
							ofParameter<bool> enabled{ "Enabled", false };
							ofParameter<string> channel{ "Channel", "Orbbec" };
							PARAM_DECLARE("Spout", enabled, channel);
						} spout;

						PARAM_DECLARE("DrawFeature", drawToWorld, draw, oscTarget, oscReceiver, spout);
					} parameters;

					ofxCvGui::PanelPtr panel;
					ofFbo fbo;
					ofTexture labelProbability;

					unique_ptr<ofxSpout::Sender> spoutSender;

					struct {
						int currentPort = -1;
						unique_ptr<ofxOscReceiver> osc;
					} receiver;

					struct {
						string currentAddress;
						int currentPort = -1;
						unique_ptr<ofxOscSender> osc;
					} sender;
				};
			}
		}
	}
}
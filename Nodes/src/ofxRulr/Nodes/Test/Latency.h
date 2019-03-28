#include "ofxRulr/Nodes/Base.h"
#include "ofxGraycode.h"

namespace ofxRulr {
	namespace Nodes {
		namespace Test {
			class Latency : public Nodes::Base {
				enum State {
				};

			public:
				Latency();
				string getTypeName() const override;
				void init();
				void update();
				ofxCvGui::PanelPtr getPanel() override;
				void populateInspector(ofxCvGui::InspectArguments &);
			protected:
				void run();

				ofxCvGui::PanelPtr panel;

				struct : ofParameterGroup {
					struct : ofParameterGroup {
						ofParameter<float> delayBetweenFrames{ "Delay between frames [ms]", 1000, 0, 30000 };
						ofParameter<float> threshold{ "Threshold", 10, 0, 255 };
						PARAM_DECLARE("Graycode step", delayBetweenFrames, threshold);
					} graycodeStep;
					PARAM_DECLARE("Latency", graycodeStep);
				} parameters;

				class Scan {
				public:
					void run(Latency * parent);

					shared_ptr<ofxGraycode::Payload::Graycode> payload;
					ofxGraycode::Encoder encoder;
					ofxGraycode::Decoder decoder;
					chrono::high_resolution_clock::time_point lastFrameSent;
				} scan;

				class Timing {
				public:
					void run(Latency * parent);
				} timing;
			};
		}
	}
}
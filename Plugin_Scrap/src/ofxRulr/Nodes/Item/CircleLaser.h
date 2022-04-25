#pragma once

#include "ofxRulr/Nodes/Base.h"
#include "ofxOsc.h"

namespace ofxRulr {
	namespace Nodes {
		namespace Item {
			/// <summary>
			/// Deprecated
			/// </summary>
			class CircleLaser : public Nodes::Base {
			public:
				CircleLaser();
				string getTypeName() const override;

				void init();
				void update();

				ofxCvGui::PanelPtr getPanel() override;

				void populateInspector(ofxCvGui::InspectArguments &);
				void drawLine(const ofPolyline & line);
				void drawPoint(const glm::vec2 & point);
				void drawCircle(const glm::vec2 & center, float radius);
				void clearDrawing();

			protected:
				void reconnect();

				struct Parameters: ofParameterGroup {
					ofParameter<string> address{ "Address", "localhost" };
					ofParameter<int> port{ "Port", 4444 };
					ofParameter<int> resolution{ "Resolution", 200 };
					PARAM_DECLARE("CircleLaser", address, port);
				};

				Parameters parameters;
				Parameters cachedParameters;

				ofImage preview;

				ofxCvGui::PanelPtr panel;
				unique_ptr<ofxOscSender> oscSender;

				ofPolyline previewLine;
			};
		}
	}
}
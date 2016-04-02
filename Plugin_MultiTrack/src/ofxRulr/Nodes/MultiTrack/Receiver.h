#pragma once

#include "ofxRulr/Nodes/Item/IDepthCamera.h"
#include "ofxMultiTrack.h"

#include "ofxRulr/Utils/ParameterGroup.h"

namespace ofxRulr {
	namespace Nodes {
		namespace MultiTrack {
			class Receiver : public Nodes::Base {
			public:
				Receiver();
				string getTypeName() const override;
				void init();
				void update();

				ofxCvGui::PanelPtr getPanel() override;

				void drawWorld() override;

				void populateInspector(ofxCvGui::InspectArguments &);
				void serialize(Json::Value &);
				void deserialize(const Json::Value &);
			protected:
				void rebuildPreviews();

				void previewsChangeCallback(ofParameter<bool> &);
				shared_ptr<ofxCvGui::Panels::Groups::Strip> panel;

				struct : ofParameterGroup {
					struct : ofParameterGroup {
						ofParameter<int> portNumber{ "Port number", 4444 };
						ofParameter<bool> connect{ "Connect", false };

						PARAM_DECLARE("Connection", portNumber, connect);
					} connection;

					struct : ofParameterGroup {
						ofParameter<bool> enabled{ "Enabled", true };

						PARAM_DECLARE("Previews", enabled);
					} previews;

					PARAM_DECLARE("Receiver", connection, previews);
				} parameters;

				shared_ptr<ofxMultiTrack::Receiver> receiver;
			};
		}
	}
}
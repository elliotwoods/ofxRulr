#pragma once

#include "ofxRulr/Nodes/Item/IDepthCamera.h"
#include "ofxRulr/Utils/ControlSocket.h"

#include "ofxMultiTrack.h"

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

				void drawWorld();

				void populateInspector(ofxCvGui::InspectArguments &);
				void serialize(Json::Value &);
				void deserialize(const Json::Value &);

				shared_ptr<ofxMultiTrack::Receiver> getReceiver() const;
			protected:
				void rebuildGui();

				void previewsChangeCallback(ofParameter<bool> &);
				shared_ptr<ofxCvGui::Panels::Groups::Strip> panel;


				struct : ofParameterGroup {
					struct : ofParameterGroup {
						ofParameter<string> senderAddress{ "Sender address", "127.0.0.1" };
						ofParameter<int> controlPort{ "Control port",  ofxMultiTrack::Ports::NodeControl };
						ofParameter<int> receivingPort{ "Receiving port",  ofxMultiTrack::Ports::NodeToServerDataRangeBegin };
						ofParameter<bool> connect{ "Connect", false };

						PARAM_DECLARE("Connection", senderAddress, controlPort, receivingPort, connect);
					} connection;

					struct : ofParameterGroup {
						ofParameter<bool> enabled{ "Enabled", true };

						PARAM_DECLARE("Previews", enabled);
					} previews;

					PARAM_DECLARE("Receiver", connection, previews);
				} parameters;

				shared_ptr<ofxMultiTrack::Receiver> receiver;
				unique_ptr<Utils::ControlSocket> controlSocket;

				ofFloatPixels depthToCameraRays;
			};
		}
	}
}
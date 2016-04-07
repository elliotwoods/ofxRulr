#pragma once

#include "ofxRulr/Nodes/Item/IDepthCamera.h"
#include "ofxMultiTrack.h"
#include "ofxRulr/Utils/ControlSocket.h"

namespace ofxRulr {
	namespace Nodes {
		namespace MultiTrack {
			class Sender : public Nodes::Base {
			public:
				Sender();
				string getTypeName() const override;
				void init();
				void update();

				void populateInspector(ofxCvGui::InspectArguments &);
				void serialize(Json::Value &);
				void deserialize(const Json::Value &);

				shared_ptr<ofxMultiTrack::Sender> getSender();
			protected:
				struct : ofParameterGroup {
					struct : ofParameterGroup {
						ofParameter<string> ipAddress{ "IP Address", "127.0.0.1" };
						ofParameter<int> port{ "Port", 2147 };
						PARAM_DECLARE("Target", ipAddress, port);
					} target;
					ofParameter<int> packetSize{ "Packet size", 16, 4096, 1e9 };
					ofParameter<int> maxSocketBufferSize{ "Max socket buffer size", 300, 0, 10000 };
					PARAM_DECLARE("Sender", packetSize, maxSocketBufferSize);
				} parameters;

				shared_ptr<ofxMultiTrack::Sender> sender;
				unique_ptr<Utils::ControlSocket> controlSocket;
			};
		}
	}
}
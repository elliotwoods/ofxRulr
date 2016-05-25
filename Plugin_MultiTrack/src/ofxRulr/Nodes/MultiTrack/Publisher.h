#pragma once

#include "ofxRulr/Nodes/Item/IDepthCamera.h"
#include "ofxMultiTrack.h"
#include "ofxRulr/Utils/ControlSocket.h"

namespace ofxRulr {
	namespace Nodes {
		namespace MultiTrack {
			class Publisher : public Nodes::Base {
			public:
				Publisher();
				string getTypeName() const override;
				void init();
				void update();

				void populateInspector(ofxCvGui::InspectArguments &);
				void serialize(Json::Value &);
				void deserialize(const Json::Value &);

				shared_ptr<ofxMultiTrack::Publisher> getPublisher();
			protected:
				void rebuild();
				void buildControlSocket();

				struct : ofParameterGroup {
					struct : ofParameterGroup {
						ofParameter<int> port{ "Port", ofxMultiTrack::Ports::NodeControl };
						ofParameter<bool> enabled{ "Enabled", false };
						PARAM_DECLARE("Control Socket", port);
					} controlSocket;

					struct : ofParameterGroup {
						ofParameter<int> port{ "Port", 5000 };
						PARAM_DECLARE("Data Socket", port);
					} dataSocket;

					struct : ofParameterGroup {
						ofParameter<int> packetSize{ "Packet size", 4096, 1024, 1e9 };
						ofParameter<int> maxSocketBufferSize{ "Max socket buffer size", 300, 0, 10000 };
						PARAM_DECLARE("Squash Buddies", packetSize, maxSocketBufferSize);
					} squashBuddies;

					PARAM_DECLARE("Publisher", controlSocket, dataSocket, squashBuddies);
				} parameters;

				shared_ptr<ofxMultiTrack::Publisher> publisher;
				unique_ptr<Utils::ControlSocket> controlSocket;
				bool droppedFrame;
			};
		}
	}
}
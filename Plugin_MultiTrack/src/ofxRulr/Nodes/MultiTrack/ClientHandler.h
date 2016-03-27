#pragma once

#include "ofxRulr/Nodes/Base.h"
#include "ofxRulr/Data/Channels/Channel.h"

#include "ofxCvGui/Panels/Scroll.h"

#include "oscpkt/udp.hh"
#include "oscpkt/oscpkt.hh"

namespace ofxRulr {
	namespace Nodes {
		namespace MultiTrack {
			class ClientHandler : public Nodes::Base {
			public:
				class Client : public ofxCvGui::Element {
				public:
					struct Frame {
						oscpkt::PacketWriter packetWriter;
					};

					Client(const string & hostName, int port, int clientIndex);
					~Client();
					
					void beginFrame();
					void add(const oscpkt::Message &);
					void endFrame();
					void pushFrame();

					int getClientIndex() const;
					const string & getHostName() const;
					int getPort() const;
				protected:
					void idleFunction();

					Data::Channels::Channel channelMask;
					string hostName;
					int port;
					int clientIndex;

					thread thread;
					bool threadRunning = false;

					oscpkt::UdpSocket socket;

					vector<shared_ptr<Frame>> outbox;
					mutex outboxMutex;

					shared_ptr<Frame> currentFrame;
				};

				ClientHandler();
				string getTypeName() const override;
				ofxCvGui::PanelPtr getPanel() override;

				void init();
				void update();
				void serialize(Json::Value &);
				void deserialize(const Json::Value &);
				void populateInspector(ofxCvGui::InspectArguments &);

				void reopenServer();

				shared_ptr<Client> addClient(const string & hostName, int port);
				void removeClient(int clientIndex);

			protected:
				void handleIncomingMessages();
				void handleAddClient(oscpkt::Message::ArgReader &, const oscpkt::SockAddr &);
				void handleRemoveClient(oscpkt::Message::ArgReader &);
				void handleSubscribe(oscpkt::Message::ArgReader &);
				void handleRequest(oscpkt::Message::ArgReader &);

				void buildOutgoingMessages();

				void rebuildView();

				ofParameter<int> port;
				ofParameter<bool> enabled;

				bool needsReopenServer = true;
				uint64_t lastReopenAttempt = 0;
				bool needsRebuildView = true;

				map<int, shared_ptr<Client>> clients;
				shared_ptr<oscpkt::UdpSocket> socketServer;

				shared_ptr<ofxCvGui::Panels::Scroll> view;
			};
		}
	}
}
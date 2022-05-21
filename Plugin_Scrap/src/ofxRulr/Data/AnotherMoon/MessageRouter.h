#pragma once

#include "Message.h"
#include "ofxOsc.h"
#include "ofThreadChannel.h"

#include <map>

namespace ofxRulr {
	namespace Data {
		namespace AnotherMoon {
			class MessageRouter {
			public:
				enum Port : int {
					Client = 4000,
					Server = 4001
				};

				~MessageRouter();

				void init(int portLocal, int portRemote);
				void close();

				bool getIncomingMessage(std::shared_ptr<IncomingMessage>& message);
				bool getIncomingAck(std::shared_ptr<AckMessageIncoming>&);
				void sendOutgoingMessage(const std::shared_ptr<OutgoingMessage>& message);

				void setAckTimeLoggingEnabled(bool);
				ofThreadChannel<int> ackTime;
			protected:
				void receiveThreadLoop();
				void sendThreadLoop();
				bool sendMessageInner(std::shared_ptr<OutgoingMessage>);

				ofThreadChannel<std::shared_ptr<IncomingMessage>> incomingMessages;
				ofThreadChannel<std::shared_ptr<OutgoingMessage>> outgoingMessages;

				// Messages being sent (messages may be sent multiple times if no ack received)
				std::vector<std::shared_ptr<OutgoingMessage>> activeOutgoingMessages;
				std::mutex activeOutgoingMessagesMutex; // inbox will delete from list on ack
				ofThreadChannel<std::shared_ptr<AckMessageOutgoing>> outgoingAcks;
				ofThreadChannel<std::shared_ptr<AckMessageIncoming>> incomingAcks;

				int portLocal;
				int portRemote;

				std::unique_ptr<ofxOscReceiver> receiver;
				std::map<Message::Address, std::unique_ptr<ofxOscSender>> senders;

				std::thread receiveThread;
				std::thread sendThread;
				bool isClosing = false;
				bool isOpen = false;

				bool ackTimeLoggingEnabled = false;
			};
		}
	}
}
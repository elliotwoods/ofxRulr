#pragma once

#include "ofxOsc.h"

namespace ofxRulr {
	namespace Data {
		namespace AnotherMoon {
			class Message : public ofxOscMessage {
			public:
				typedef int64_t Index;
				typedef std::string Address;

				Message()
				{

				}

				Message(const ofxOscMessage& message)
				: ofxOscMessage(message)
				{

				}

				Index index;

				static const std::string ackAddress;;
			};

			class IncomingMessage : public Message {
			public:
				IncomingMessage(const ofxOscMessage& message);
				bool isValid() const;

			protected:
				bool valid = false;
			};

			class OutgoingMessage : public Message {
			public:
				OutgoingMessage(const std::string& targetHost);
				
				const std::string& getTargetHost() const;

				virtual void markAsSent() = 0;
				virtual bool getShouldSend() const = 0;
				virtual bool getShouldDestroy() const = 0;
			protected:
				static size_t nextIndex;
				std::string targetHost;
				size_t sendCount = 0;
			};

			class OutgoingMessageOnce : public OutgoingMessage {
			public:
				OutgoingMessageOnce(const std::string& targetHost);

				void markAsSent() override;
				bool getShouldSend() const override;
				bool getShouldDestroy() const override;
			};

			class OutgoingMessageRetry : public OutgoingMessage {
			public:
				OutgoingMessageRetry(const std::string& targetHost
					, const std::chrono::system_clock::duration& retryDuration
					, const std::chrono::system_clock::duration& retryPeriod);

				void markAsSent() override;
				bool getShouldSend() const override;
				bool getShouldDestroy() const override;
			protected:
				std::chrono::system_clock::time_point lastSendTime; // value initialise to zeros
				std::chrono::system_clock::duration retryDuration;
				std::chrono::system_clock::time_point retryDeadline;
			};

			/// <summary>
			/// Acknowledge a message
			/// </summary>
			class AckMessageOutgoing : public OutgoingMessageOnce {
			public:
				// Create a response to incoming message
				AckMessageOutgoing(const IncomingMessage&);

			};

			class AckMessageIncoming : public IncomingMessage {
			public:
				AckMessageIncoming(const ofxOscMessage&);
			};
		}
	}
}
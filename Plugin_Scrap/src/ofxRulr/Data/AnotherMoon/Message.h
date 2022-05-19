#pragma once

#include "ofxOsc.h"

#include <future>

namespace ofxRulr {
	namespace Data {
		namespace AnotherMoon {
			class OutgoingMessageRetry;

			class Message : public ofxOscMessage {
			public:
				typedef int64_t Index;
				typedef std::string Address;
				typedef std::string HostName;

				struct TimeoutException : std::exception {
					TimeoutException(const std::chrono::system_clock::duration&
						, const Address&
						, const HostName& target);
					TimeoutException(const OutgoingMessageRetry&);
					char const* what() const noexcept override;

					const std::chrono::system_clock::duration duration;
					const Address& address;
					const HostName& target;
					char message[256];
				};

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
				OutgoingMessage(const HostName& targetHost);
				
				const HostName& getTargetHost() const;

				virtual void markAsSent() = 0;
				virtual bool getShouldSend() const = 0;
				virtual bool getShouldDestroy() const = 0;

				std::promise<void> onSent;
			protected:
				static size_t nextIndex;
				HostName targetHost;
				size_t sendCount = 0;
			};

			class OutgoingMessageOnce : public OutgoingMessage {
			public:
				OutgoingMessageOnce(const HostName& targetHost);

				void markAsSent() override;
				bool getShouldSend() const override;
				bool getShouldDestroy() const override;
			};

			class OutgoingMessageRetry : public OutgoingMessage {
			public:
				OutgoingMessageRetry(const HostName& targetHost
					, const std::chrono::system_clock::duration& retryDuration
					, const std::chrono::system_clock::duration& retryPeriod);

				void markAsSent() override;
				bool getShouldSend() const override;
				bool getShouldDestroy() const override;

				std::chrono::system_clock::duration getRetryDuration() const;

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
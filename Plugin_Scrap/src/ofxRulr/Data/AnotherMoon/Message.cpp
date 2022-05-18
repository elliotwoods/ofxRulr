#include "Message.h"

using namespace std;

namespace ofxRulr {
	namespace Data {
		namespace AnotherMoon {
			//----------
			const string Message::ackAddress = "/ack";

#pragma mark IncomingMessage
			//-----------
			IncomingMessage::IncomingMessage(const ofxOscMessage& message)
				: Message(message) {
				// Check message is valid
				if (this->getArgType(0) != ofxOscArgType::OFXOSC_TYPE_INT64) {
					this->valid = false;
					return;
				}

				this->index = this->getArgAsInt64(0);
				this->valid = true;
			}

			//-----------
			bool
				IncomingMessage::isValid() const
			{
				return this->valid;
			}

#pragma mark OutgoingMessage
			//-----------
			size_t OutgoingMessage::nextIndex = 0;

			//----------
			OutgoingMessage::OutgoingMessage(const string& targetHost)
				: targetHost(targetHost)
			{
				this->index = this->nextIndex++;
				this->addInt64Arg(this->nextIndex);
				this->targetHost = targetHost;
			}

			//----------
			const string&
				OutgoingMessage::getTargetHost() const
			{
				return this->targetHost;
			}

#pragma mark OutgoingMessageOnce
			//----------
			OutgoingMessageOnce::OutgoingMessageOnce(const string& targetHost)
				: OutgoingMessage(targetHost)
			{

			}

			

			//----------
			void
				OutgoingMessageOnce::markAsSent()
			{
				this->sendCount++;
			}

			//----------
			bool
				OutgoingMessageOnce::getShouldSend() const
			{
				return this->sendCount == 0;
			}

			//----------
			bool
				OutgoingMessageOnce::getShouldDestroy() const
			{
				return this->sendCount > 0;
			}

#pragma mark OutgoingMessageRetry
			//----------
			OutgoingMessageRetry::OutgoingMessageRetry(const string& targetHost
				, const std::chrono::system_clock::duration& retryDuration
				, const std::chrono::system_clock::duration& retryPeriod)
				: OutgoingMessage(targetHost)
				, retryDuration(retryDuration)
				, retryDeadline(std::chrono::system_clock::now() + retryPeriod)
			{
			}

			//----------
			void
				OutgoingMessageRetry::markAsSent()
			{
				this->sendCount++;
				this->lastSendTime = std::chrono::system_clock::now();
			}


			//----------
			bool
				OutgoingMessageRetry::getShouldSend() const
			{
				// Always send if never sent
				if (this->sendCount == 0) {
					return true;
				}

				// Otherwise check retry period and deadline
				auto now = std::chrono::system_clock::now();
				return now - this->lastSendTime > this->retryDuration
					&& now < retryDeadline;
			}

			//----------
			bool
				OutgoingMessageRetry::getShouldDestroy() const
			{
				// Don't destroy if not sent
				if (this->sendCount == 0) {
					return false;
				}

				// Otherwise check deadline
				auto now = std::chrono::system_clock::now();
				return now > retryDeadline;
			}

#pragma mark AckknowledgeMessage
			//----------
			AckMessageOutgoing::AckMessageOutgoing(const IncomingMessage& incomingMessage)
				: OutgoingMessageOnce(incomingMessage.getRemoteHost())
			{
				this->index = incomingMessage.index;
				this->setAddress(this->ackAddress);
			}

			//----------
			AckMessageIncoming::AckMessageIncoming(const ofxOscMessage& incomingMessage)
				: IncomingMessage(incomingMessage)
			{
				this->setAddress(this->ackAddress);
			}
		}
	}
}

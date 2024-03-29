#include "Message.h"

using namespace std;

namespace ofxRulr {
	namespace Data {
		namespace AnotherMoon {
#pragma mark Message
			//----------
			const string Message::ackAddress = "/ack";

			//----------
			Message::TimeoutException::TimeoutException(const std::chrono::system_clock::duration& duration
				, const Address& address
				, const HostName& target)
				: duration(duration)
				, address(address)
				, target(target)
			{
				sprintf(this->message, "Timeout sending to '%s:%s', (%d ms)"
					, this->target.c_str()
					, this->address.c_str()
					, (int32_t) (std::chrono::duration_cast<std::chrono::milliseconds>(this->duration)).count());
			}

			//----------
			Message::TimeoutException::TimeoutException(const OutgoingMessageRetry& outgoingMessageRetry)
				: TimeoutException(outgoingMessageRetry.getRetryDuration()
					, outgoingMessageRetry.getAddress()
					, outgoingMessageRetry.getTargetHost())
			{
			}

			//----------
			char const *
				Message::TimeoutException::what() const noexcept
			{
				return this->message;
			}

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
			OutgoingMessage::OutgoingMessage(const HostName& targetHost)
				: targetHost(targetHost)
			{
				this->index = this->nextIndex++;
				this->addInt64Arg(this->index);
				this->targetHost = targetHost;
			}

			//----------
			const Message::HostName&
				OutgoingMessage::getTargetHost() const
			{
				return this->targetHost;
			}

			//----------
			chrono::system_clock::duration
				OutgoingMessage::getAge() const
			{
				return chrono::system_clock::now() - this->birthTime;
			}

#pragma mark OutgoingMessageOnce
			//----------
			OutgoingMessageOnce::OutgoingMessageOnce(const HostName& targetHost)
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
			OutgoingMessageRetry::OutgoingMessageRetry(const HostName& targetHost
				, const std::chrono::system_clock::duration& retryDuration
				, const std::chrono::system_clock::duration& retryPeriod)
				: OutgoingMessage(targetHost)
				, retryDuration(retryDuration)
				, retryPeriod(retryPeriod)
				, retryDeadline(std::chrono::system_clock::now() + retryDuration)
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
				return now - this->lastSendTime > this->retryPeriod
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

			//----------
			std::chrono::system_clock::duration
				OutgoingMessageRetry::getRetryDuration() const
			{
				return this->retryDuration;
			}

#pragma mark AckknowledgeMessage
			//----------
			AckMessageOutgoing::AckMessageOutgoing(const IncomingMessage& incomingMessage)
				: OutgoingMessageOnce(incomingMessage.getRemoteHost())
			{
				this->index = incomingMessage.index;

				// Remove existing ID (inherited)
				this->clear();
				this->setAddress(this->ackAddress);
				this->addInt64Arg(this->index);
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

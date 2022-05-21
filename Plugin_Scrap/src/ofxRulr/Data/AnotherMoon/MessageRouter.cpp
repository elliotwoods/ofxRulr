#include "MessageRouter.h"

using namespace std;

namespace ofxRulr {
	namespace Data {
		namespace AnotherMoon {
			//----------
			MessageRouter::~MessageRouter()
			{
				this->close();
			}

			//----------
			void
				MessageRouter::init(int portLocal, int portRemote)
			{
				this->portLocal = portLocal;
				this->portRemote = portRemote;

				this->close();

				this->receiveThread = std::thread([this]() {
					while (!this->isClosing) {
						this->receiveThreadLoop();
					}
					});

				this->sendThread = std::thread([this]() {
					while (!this->isClosing) {
						this->sendThreadLoop();
					}
					});

				this->isOpen = true;
			}

			//----------
			void
				MessageRouter::close()
			{
				if (this->isOpen) {
					this->isClosing = true;
					this->receiveThread.join();
					this->sendThread.join();
					this->isOpen = false;
				}
			}

			//----------
			bool
				MessageRouter::getIncomingMessage(std::shared_ptr<IncomingMessage>& message)
			{
				return this->incomingMessages.tryReceive(message);
			}

			//----------
			bool
				MessageRouter::getIncomingAck(std::shared_ptr<AckMessageIncoming>& message)
			{
				return this->incomingAcks.tryReceive(message);
			}

			//----------
			void
				MessageRouter::sendOutgoingMessage(const std::shared_ptr<OutgoingMessage>& message)
			{
				this->outgoingMessages.send(message);
			}

			//----------
			void
				MessageRouter::setAckTimeLoggingEnabled(bool enabled)
			{
				this->ackTimeLoggingEnabled = enabled;
			}

			//----------
			void
				MessageRouter::receiveThreadLoop()
			{
				// Check server settings
				if (this->receiver) {
					if (this->receiver->getPort() != this->portLocal) {
						this->receiver.reset();
					}
				}

				// Create receiver if required
				if (!this->receiver) {
					this->receiver = make_unique<ofxOscReceiver>();
					this->receiver->setup(this->portLocal);
				}

				// Take incoming messages
				ofxOscMessage oscMessage;
				if (this->receiver->getNextMessage(oscMessage)) {
					auto incomingMessage = make_shared<IncomingMessage>(oscMessage);

					// Check if it's an ack
					if (incomingMessage->getAddress() == Message::ackAddress) {
						// RECEIVED AN ACK

						auto ackIncoming = make_shared<AckMessageIncoming>(*incomingMessage);

						// Delete outgoing messages waiting for this ack
						{
							this->activeOutgoingMessagesMutex.lock();
							{
								// Delete all outbox messages waiting for this ack
								for (auto it = this->activeOutgoingMessages.begin(); it != this->activeOutgoingMessages.end();)
								{
									if ((*it)->index == ackIncoming->index)
									{
										// Alert listeners that it worked
										(*it)->onSent.set_value();

										// Send to histogram if enabled
										if (this->ackTimeLoggingEnabled) {
											auto ackTimeMillis = chrono::duration_cast<chrono::milliseconds>((*it)->getAge()).count();
											this->ackTime.send(ackTimeMillis);
										}

										// Delete it from the queue
										it = this->activeOutgoingMessages.erase(it);
									}
									else {
										it++;
									}
								}
							}
							this->activeOutgoingMessagesMutex.unlock();
						}

						// Transmit ACK to main thread
						this->incomingAcks.send(make_shared<AckMessageIncoming>(oscMessage));
					}
					else {
						// RECEIVED A STANDARD MESSAGE

						// Send message to main thread
						this->incomingMessages.send(incomingMessage);

						// Send an ACK
						this->outgoingAcks.send(make_shared<AckMessageOutgoing>(*incomingMessage));
					}
				}
				else {
					ofSleepMillis(10);
				}
			}

			//----------
			void
				MessageRouter::sendThreadLoop()
			{
				// Receive messages to send from main thread
				{
					std::shared_ptr<OutgoingMessage> activeOutgoingMessage;
					while (this->outgoingMessages.tryReceive(activeOutgoingMessage, 10)) {

						// Strip messages from outbox, with same address as new message (if it's not an ACK)
						if (activeOutgoingMessage->getAddress() != Message::ackAddress) {

							this->activeOutgoingMessagesMutex.lock();
							{
								for (auto it = this->activeOutgoingMessages.begin(); it != this->activeOutgoingMessages.end(); )
								{
									const auto& otherExistingActiveMessage = *it;
									// If matching address and target host
									if (otherExistingActiveMessage->getAddress() == activeOutgoingMessage->getAddress()
										&& otherExistingActiveMessage->getTargetHost() == activeOutgoingMessage->getTargetHost()) {
										// erase the old one
										it = this->activeOutgoingMessages.erase(it);
									}
									else {
										it++;
									}
								}
							}
							this->activeOutgoingMessagesMutex.unlock();
						}

						// Put this message into outgoing loop
						this->activeOutgoingMessagesMutex.lock();
						{
							this->activeOutgoingMessages.push_back(activeOutgoingMessage);
						}
						this->activeOutgoingMessagesMutex.unlock();
					}
				}

				// Process the outgoing active messages
				this->activeOutgoingMessagesMutex.lock();
				{
					for (auto it = this->activeOutgoingMessages.begin(); it != this->activeOutgoingMessages.end(); ) {
						auto outgoingMessage = *it;
						if (outgoingMessage->getShouldDestroy()) {
							auto outgoingMessageRetry = dynamic_pointer_cast<OutgoingMessageRetry>(outgoingMessage);
							if (outgoingMessageRetry) {
								// The message has expired - send an exception
								outgoingMessage->onSent.set_exception(make_exception_ptr(Message::TimeoutException(*outgoingMessageRetry)));
							}
							
							it = this->activeOutgoingMessages.erase(it);
						}
						else {
							// Check (e.g. retry period)
							if (outgoingMessage->getShouldSend()) {
								if (this->sendMessageInner(outgoingMessage)) {
									outgoingMessage->markAsSent();
								}
							}
							it++;
						}
					}
				}
				this->activeOutgoingMessagesMutex.unlock();

				// Send the ACKS
				{
					shared_ptr<AckMessageOutgoing> ackMessage;
					while (this->outgoingAcks.tryReceive(ackMessage)) {
						this->sendMessageInner(ackMessage);
					}
				}
			}

			//----------
			bool
				MessageRouter::sendMessageInner(shared_ptr<OutgoingMessage> outgoingMessage)
			{
				// Refresh the sender if required
				{
					// Find existing sender
					auto findSender = this->senders.find(outgoingMessage->getTargetHost());
					if (findSender != this->senders.end()) {
						auto& sender = findSender->second;

						// Check the send port matches
						if (sender) {
							if (this->portRemote != sender->getPort()) {
								this->senders.erase(findSender);
							}
						}
					}
				}

				// Check if need create sender
				{
					auto findSender = this->senders.find(outgoingMessage->getTargetHost());
					if (findSender == this->senders.end()) {
						auto it = this->senders.emplace(outgoingMessage->getTargetHost(), make_unique<ofxOscSender>());
						auto& sender = it.first->second;

						ofxOscSenderSettings settings;
						{
							settings.host = outgoingMessage->getTargetHost();
							settings.port = this->portRemote;
						}

						if (!sender->setup(settings)) {
							this->senders.erase(it.first);
							return false;
						}
					}
				}

				// Send message
				{
					auto findSender = this->senders.find(outgoingMessage->getTargetHost());
					if (findSender != this->senders.end()) {
						findSender->second->sendMessage(*outgoingMessage);
						return true;
					}
					else {
						return false;
					}
				}
			}
		}
	}
}
#include "pch_MultiTrack.h"
#include "ClientHandler.h"

#include "ofxRulr/Nodes/Data/Channels/Database.h"

using namespace ofxRulr::Data::Channels;
using namespace ofxCvGui;
using namespace oscpkt;

namespace ofxRulr {
	namespace Nodes {
		namespace MultiTrack {
#pragma mark Client
			//----------
			ClientHandler::Client::Client(const string & hostName, int port, int clientIndex) : 
			channelMask("") {
				this->threadRunning = true;
				this->hostName = hostName;
				this->port = port;
				this->clientIndex = clientIndex;

				this->thread = std::thread([&]() {
					this->socket.connectTo(this->hostName, this->port);

					while (threadRunning) {
						this->idleFunction();
					}
				});

				this->onDraw += [this](DrawArguments & args) {
					auto bounds = args.localBounds;
					bounds.x += 10;
					bounds.width -= 20;
					bounds.height -= 10;

					stringstream message;
					message << "Client #" << this->clientIndex << endl;
					message << this->hostName << ":" << this->port;
					ofxCvGui::Utils::drawText(message.str(), bounds);
				};
				this->setHeight(60);
			}

			//----------
			ClientHandler::Client::~Client() {
				this->threadRunning = false;
				this->thread.join();
			}

			//----------
			void ClientHandler::Client::beginFrame() {
				this->currentFrame = make_shared<Frame>();
				Message message("/begin");
				this->currentFrame->packetWriter.startBundle();
				this->currentFrame->packetWriter.addMessage(message);
			}

			//----------
			void ClientHandler::Client::add(const oscpkt::Message & message) {
				if (this->currentFrame) {
					this->currentFrame->packetWriter.addMessage(message);

					if (this->currentFrame->packetWriter.packetSize() > 2048) {
						this->currentFrame->packetWriter.endBundle();
						this->pushFrame();
						this->currentFrame = make_shared<Frame>();
						this->currentFrame->packetWriter.startBundle();
					}
				}
			}

			//----------
			void ClientHandler::Client::endFrame() {
				if (this->currentFrame) {
					Message message("/end");
					this->currentFrame->packetWriter.addMessage(message);
					this->currentFrame->packetWriter.endBundle();
					this->pushFrame();
				}
			}

			//----------
			void ClientHandler::Client::pushFrame() {
				this->outboxMutex.lock();
				this->outbox.push_back(this->currentFrame);
				this->outboxMutex.unlock();

				this->currentFrame.reset();
			}

			//----------
			int ClientHandler::Client::getClientIndex() const {
				return this->clientIndex;
			}

			//----------
			const string & ClientHandler::Client::getHostName() const {
				return this->hostName;
			}

			//----------
			int ClientHandler::Client::getPort() const {
				return this->port;
			}

			//----------
			void ClientHandler::Client::idleFunction() {
				//make an empty copy of outbox which we'll swap in
				vector<shared_ptr<Frame>> outbox;

				//move the outbox into this thread
				this->outboxMutex.lock();
				outbox.swap(this->outbox);
				this->outboxMutex.unlock();

				//leave early if there's nothing to party with
				for(auto frame : outbox) {
					this->socket.sendPacket(frame->packetWriter.packetData(), frame->packetWriter.packetSize());
				}
				outbox.clear();
			}

#pragma mark ClientHandler
			//----------
			ClientHandler::ClientHandler() {
				RULR_NODE_INIT_LISTENER;
			}

			//----------
			string ClientHandler::getTypeName() const {
				return "MultiTrack::ClientHandler";
			}

			//----------
			PanelPtr ClientHandler::getPanel() {
				return this->view;
			}
			
			//----------
			void ClientHandler::init() {
				RULR_NODE_UPDATE_LISTENER;
				RULR_NODE_SERIALIZATION_LISTENERS;
				RULR_NODE_INSPECTOR_LISTENER;

				this->addInput<Data::Channels::Database>();

				this->port.set("Port", 2046);
				this->enabled.set("Enabled", true);

				this->view = make_shared<Panels::Scroll>();
			}

			//----------
			void ClientHandler::update() {
				if (this->enabled && !this->socketServer) {
					this->needsReopenServer = true;
				}

				this->handleIncomingMessages();
				this->buildOutgoingMessages();

				if (this->needsReopenServer) {
					this->reopenServer();
				}

				if (this->needsRebuildView) {
					this->rebuildView();
				}
			}

			//----------
			void ClientHandler::serialize(Json::Value & json) {
				Utils::Serializable::serialize(json, this->port);
				Utils::Serializable::serialize(json, this->enabled);
			}

			//----------
			void ClientHandler::deserialize(const Json::Value & json) {
				Utils::Serializable::deserialize(json, this->port);
				Utils::Serializable::deserialize(json, this->enabled);
				this->needsReopenServer = true;
			}

			//----------
			void ClientHandler::populateInspector(ofxCvGui::InspectArguments & inspectArgs) {
				auto inspector = inspectArgs.inspector;

				inspector->addEditableValue(this->port);
				inspector->addEditableValue(this->enabled);

				inspector->addIndicator("Server bound", [this]() {
					if (this->socketServer) {
						return true;
					}
					else {
						return false;
					}
				});

				inspector->addLiveValue<size_t>("Client count", [this]() {
					return this->clients.size();
				});
			}

			//----------
			void ClientHandler::reopenServer() {
				auto now = ofGetElapsedTimeMillis();

				//don't do anything if we tried to reopen less than 2s ago
				if (now - this->lastReopenAttempt < 2000) {
					return;
				}

				//if we're already open, then close
				if (this->socketServer) {
					this->socketServer->close();
					this->socketServer.reset();
				}

				//try and open
				this->socketServer = make_shared<UdpSocket>();
				this->socketServer->bindTo(this->port);

				//if we failed, clear the connection
				if (!this->socketServer->isOk()) {
					ofLogError("MultiTrack::ClientHandler") << "Failed to bind server on port " << this->port;
					this->socketServer.reset();
				}
				else {
					this->needsReopenServer = false;
				}

				this->lastReopenAttempt = now;
			}

			//----------
			shared_ptr<ClientHandler::Client> ClientHandler::addClient(const string & hostName, int port) {
				//choose the clientIndex
				int clientIndex = 0;
				if (!this->clients.empty()) {
					//next index is current last index + 1
					clientIndex = this->clients.rbegin()->first + 1;
				}

				//clean any existing clients which have the same settings
				for (auto it = this->clients.begin(); it != this->clients.end(); ) {
					if (it->second->getHostName() == hostName && it->second->getPort() == port) {
						//this has same settings as one which we will add now
						it = this->clients.erase(it);
					}
					else {
						it++;
					}
				}

				//make and keep the new client
				auto client = make_shared<Client>(hostName, port, clientIndex);
				this->clients[clientIndex] = client;

				this->needsRebuildView = true;

				return client;
			}

			//----------
			void ClientHandler::removeClient(int clientIndex) {
				for (auto it = this->clients.begin(); it != this->clients.end();) {
					if (clientIndex == -1 || it->first == clientIndex) {
						it = this->clients.erase(it);
					}
					else {
						it++;
					}
				}

				this->needsRebuildView = true;
			}

			//----------
			void ClientHandler::handleIncomingMessages() {
				if (!this->socketServer) {
					return;
				}

				PacketReader reader;
				while (this->socketServer->receiveNextPacket(0)) {
					//initialise the reader
					reader.init(this->socketServer->packetData(), this->socketServer->packetSize());

					Message * message;
					while (reader.isOk()) {
						//get next message
						message = reader.popMessage();
						if (!message) {
							break;
						}

						{
							auto handler = message->match("/addClient");
							if (handler) {
								this->handleAddClient(handler, this->socketServer->packetOrigin());
							}
						}

						{
							auto handler = message->match("/removeClient");
							if (handler) {
								this->handleRemoveClient(handler);
							}
						}

						{
							auto handler = message->match("/subscribe");
							if (handler) {
								this->handleSubscribe(handler);
							}
						}

						{
							auto handler = message->match("/request");
							if (handler) {
								this->handleRequest(handler);
							}
						}
					}
				}
			}

			//----------
			void serializeChannel(shared_ptr<ClientHandler::Client> client, shared_ptr<Channel> channel, string prefix = "") {
				for (const auto & subChannelIt : channel->getSubChannels()) {
					auto name = subChannelIt.first;
					auto subChannel = subChannelIt.second;

					auto address = prefix + "/" + name;
					Message message(address);

					auto valueType = subChannel->getValueType();
					switch (valueType) {
					case Channel::Type::Int32:
					{
						message.pushInt32(subChannel->getValue<int32_t>());
						break;
					}
					case Channel::Type::Int64:
					{
						message.pushInt64(subChannel->getValue<int64_t>());
						break;
					}
					case Channel::Type::UInt32:
					{
						message.pushInt32(subChannel->getValue<uint32_t>());
						break;
					}
					case Channel::Type::UInt64:
					{
						message.pushInt64(subChannel->getValue<uint64_t>());
						break;
					}
					case Channel::Type::Float:
					{
						message.pushFloat(subChannel->getValue<float>());
						break;
					}
					case Channel::Type::String:
					{
						message.pushStr(subChannel->getValue<string>());
						break;
					}
					case Channel::Type::Vec3f:
					{
						auto & value = subChannel->getValue<ofVec3f>();
						for (int i = 0; i < 3; i++) {
							message.pushFloat(value[i]);
						}
						break;
					}
					case Channel::Type::Vec4f:
					{
						auto & value = subChannel->getValue<ofVec4f>();
						for (int i = 0; i < 4; i++) {
							message.pushFloat(value[i]);
						}
						break;
					}
					default:
						break;
					}
					client->add(message);

					serializeChannel(client, subChannel, address);
				}
			}

			//----------
			void ClientHandler::buildOutgoingMessages() {
				auto databaseNode = this->getInput<Data::Channels::Database>();

				for (auto clientIt : this->clients) {
					auto client = clientIt.second;

					client->beginFrame();

					{
						Message message("/clientIndex");
						message.pushInt32(clientIt.first);
						client->add(message);
					}

					if (databaseNode) {
						auto rootChannel = databaseNode->getRootChannel();
						serializeChannel(client, rootChannel);
					}

					client->endFrame();
				}
			}

			//----------
			void ClientHandler::handleAddClient(Message::ArgReader & reader, const SockAddr & origin) {
				int portNumber = origin.getPort();
				string hostName;

				{
					char hostNameChar[512];
					auto error = getnameinfo(&origin.addr(), sizeof(origin.addr()), hostNameChar, sizeof(hostNameChar), 0, 0, NI_NUMERICHOST);
					if (error == 0) {
						hostName = string(hostNameChar);
					}
				}

				if (!reader.isOkNoMoreArgs()) {
					//we have args
					reader.popInt32(portNumber);
					if (!reader.isOkNoMoreArgs()) {
						reader.popStr(hostName);
					}
				}

				if (reader.isOk()) {
					this->addClient(hostName, portNumber);
				}
			}

			//----------
			void ClientHandler::handleRemoveClient(Message::ArgReader & reader) {
				int clientIndex;
				if (!reader.popInt32(clientIndex).isOk()) {
					clientIndex = -1;
				}
				this->removeClient(clientIndex);
			}

			//----------
			void ClientHandler::handleSubscribe(Message::ArgReader & reader) {

			}

			//----------
			void ClientHandler::handleRequest(Message::ArgReader & reader) {

			}

			//----------
			void ClientHandler::rebuildView() {
				this->view->clear();
				this->view->add(new Widgets::Title("Clients", Widgets::Title::Level::H2));
				for (auto client : this->clients) {
					this->view->add(client.second);
				}

				this->needsRebuildView = false;
			}
		}
	}
}
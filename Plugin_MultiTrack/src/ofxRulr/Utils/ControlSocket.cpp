#include "pch_MultiTrack.h"
#include "ControlSocket.h"

namespace ofxRulr {
	namespace Utils {
		//----------
		ControlSocket::ControlSocket() {
			this->addHandler("heartbeat", [this](const Json::Value &) {
				this->lastHeartbeatReceived = chrono::system_clock::now();
			});
		}

		//----------
		void ControlSocket::init(int port) {
			this->type = Bind;
			this->bind.port = port;
			this->socket.reset();
		}

		//----------
		void ControlSocket::init(const string & address, int port) {
			this->type = Connect;
			this->connect.address = address;
			this->connect.port = port;
			this->socket.reset();
		}

		//----------
		void ControlSocket::update() {
			if (!this->socket) {
				try {
					auto socket = make_unique<ofxZmqPair>();
					switch (this->type)
					{
					case Connect:
						socket->connect("tcp://" + this->connect.address + ":" + ofToString(this->connect.port));
						break;
					case Bind:
					default:
						socket->bind("tcp://*:" + ofToString(this->bind.port));
						break;
					}

					this->socket = move(socket);
					this->sendHeartbeat();
				}
				catch (...) {
					ofLogError("ControlSocket") << "Failed to make connection";
				}
			}

			if (!this->socket) {
				return;
			}

			if (this->getTimeSinceLastHeartbeatSent() > chrono::milliseconds(Constants::HeartbeatPeriodMS)) {
				this->sendHeartbeat();
			}

			while (this->socket->hasWaitingMessage()) {
				string message;
				this->socket->getNextMessage(message);
				Json::Reader reader;
				Json::Value json;
				if (!reader.parse(message, json)) {
					ofLogWarning("ContolSocket") << "Failed to parse message";
					continue;
				}

				if (!json["actionName"].empty()) {
					auto actionName = json["actionName"].asString();
					const auto & arguments = json["arguments"];

					auto findHandler = this->handlers.find(actionName);
					if (findHandler == this->handlers.end()) {
						ofLogWarning("ContolSocket") << "No handler found for action [" << actionName << "]";
					}
					else {
						findHandler->second(arguments);
					}
				}
			}
		}

		//----------
		void ControlSocket::addHandler(const string & actionName, MessageHandler && action) {
			this->handlers.emplace(actionName, action);
		}

		//----------
		void ControlSocket::sendMessage(const string & actionName, const Json::Value & arguments) {
			if (!this->socket) {
				ofLogError("ControlSocket") << "Cannot send messages until socket is open";
				return;
			}
			
			Json::FastWriter writer;

			Json::Value messageJson;
			messageJson["actionName"] = actionName;
			if (!arguments.empty()) {
				messageJson["arguments"] = arguments;
			}
			const auto message = writer.write(messageJson);
			this->socket->send(message, true);
		}

		//----------
		chrono::system_clock::duration ControlSocket::getTimeSinceLastHeartbeatReceived() const {
			return chrono::system_clock::now() - this->lastHeartbeatReceived;
		}

		//----------
		chrono::system_clock::duration ControlSocket::getTimeSinceLastHeartbeatSent() const {
			return chrono::system_clock::now() - this->lastHeartbeatSent;
		}

		//----------
		void ControlSocket::sendHeartbeat() {
			this->sendMessage("heartbeat");
			this->lastHeartbeatSent = chrono::system_clock::now();
		}

		//----------
		bool ControlSocket::isSocketActive() const {
			if (this->socket) {
				return true;
			} else {
				return false;
			}
		}
	}
}
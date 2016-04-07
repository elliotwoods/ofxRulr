#include "pch_MultiTrack.h"
#include "ControlSocket.h"

namespace ofxRulr {
	namespace Utils {
		//----------
		ControlSocket::ControlSocket(int port) {
			this->socket.bind("tcp://*:" + ofToString(port));
		}

		//----------
		ControlSocket::ControlSocket(const string & address, int port) {
			this->socket.connect("tcp://" + address + ":" + ofToString(port));
		}

		//----------
		void ControlSocket::update() {
			if (this->socket.hasWaitingMessage()) {
				string message;
				while (this->socket.getNextMessage(message)) {
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
					}
				}
			}
		}

		//----------
		void ControlSocket::addHandler(const string & actionName, Handler && action) {
			this->handlers.emplace(actionName, action);
		}

		//----------
		void ControlSocket::sendMessage(const string & actionName, const Json::Value & arguments) {
			Json::FastWriter writer;

			Json::Value messageJson;
			messageJson["actionName"] = actionName;
			if (!arguments.empty()) {
				messageJson["arguments"] = arguments;
			}
			const auto message = writer.write(messageJson);
			this->socket.send(message);
		}
	}
}
#pragma once

#include "ofxZmq.h"
#include "ofxRulr/Utils/Serializable.h"

namespace ofxRulr {
	namespace Utils {
		class ControlSocket {
		public:
			typedef function<void(const Json::Value &)> Handler;

			ControlSocket(int port = 2146);
			ControlSocket(const string & address, int port = 2146);

			void update();

			void addHandler(const string & actionName, Handler && action);

			void sendMessage(const string & actionName, const Json::Value & = Json::Value());

		protected:
			ofxZmqPair socket;
			map<string, Handler> handlers;
		};
	}
}
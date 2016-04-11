#pragma once

#include "ofxZmq.h"
#include "ofxRulr/Utils/Serializable.h"

namespace ofxRulr {
	namespace Utils {
		class ControlSocket {
		public:
			enum Constants : uint64_t {
				HeartbeatPeriodMS = 1000,
				HeartbeatLossThresholdMS = 3000
			};

			typedef function<void(const Json::Value &)> MessageHandler;

			ControlSocket();
			void init(int port);
			void init(const string & address, int port);

			void update();

			void addHandler(const string & actionName, MessageHandler && action);

			void sendHeartbeat();
			void sendMessage(const string & actionName, const Json::Value & = Json::Value());
			
			chrono::system_clock::duration getTimeSinceLastHeartbeatReceived() const;
			chrono::system_clock::duration getTimeSinceLastHeartbeatSent() const;

			bool isSocketActive() const;
		protected:
			struct {
				string address;
				int port;
			} connect;

			struct {
				int port;
			} bind;

			enum {
				Connect,
				Bind
			} type;

			unique_ptr<ofxZmqPair> socket;
			chrono::system_clock::time_point lastHeartbeatSent;
			chrono::system_clock::time_point lastHeartbeatReceived;

			map<string, MessageHandler> handlers;
		};
	}
}
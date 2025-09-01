#pragma once

#include "ofxRulr.h"
#include "ofxOsc.h"

#include "ofxRulr/Models/Reworld/AxisAngles.h"

namespace ofxRulr {
	namespace Nodes {
		namespace Reworld {
			class Router : public Nodes::Base {
			public:
				struct Address {
					int column;
					uint8_t portal;
					bool operator<(const Address&) const;
				};

				Router();
				string getTypeName() const override;

				void init();
				void update();

				string getBaseURI() const;
				string getBaseURI(const Address&) const;

				nlohmann::json loadURI(const string& uri);

				void test();
				void setPosition(const Address&, const glm::vec2&);
				glm::vec2 getPosition(const Address&);
				glm::vec2 getTargetPosition(const Address&);
				bool isInPosition(const Address&);
				void poll(const Address&);
				void push(const Address&);

				void sendAxisValues(const map<Address, Models::Reworld::AxisAngles<float>>&);
				void sendOSCMessageToAll(string oscAddress);
				void sendOSCMessageToColumn(int columnIndex, string oscAddress);
				void sendOSCMessageToModule(Address moduleAddress, string oscAddress);
			protected:

				struct : ofParameterGroup {
					ofParameter<string> hostname{ "Hostname", "localhost" };
					struct : ofParameterGroup {
						ofParameter<int> port{ "Port", 8080 };
						PARAM_DECLARE("REST", port);
					} rest;

					struct : ofParameterGroup {
						ofParameter<int> port{ "Port", 4000 };
						ofParameter<int> maxPortalsPerMessage{ "Max portals per message", 64 };
						PARAM_DECLARE("OSC", port, maxPortalsPerMessage);
					} osc;
					PARAM_DECLARE("Router", hostname, rest, osc);
				} parameters;

				shared_ptr<ofxOscSender> oscSender;
			};
		}
	}
}
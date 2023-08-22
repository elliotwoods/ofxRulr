#pragma once

#include "ofxRulr.h"
#include "ofxOsc.h"

namespace ofxRulr {
	namespace Nodes {
		namespace Reworld {
			class Router : public Nodes::Base {
			public:
				struct Address {
					int column;
					uint8_t portal;
				};

				Router();
				string getTypeName() const override;

				void init();

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

			protected:

				struct : ofParameterGroup {
					ofParameter<string> hostname{ "Hostname", "localhost" };
					ofParameter<int> port{ "Port", 8080};
					PARAM_DECLARE("Router", hostname, port);
				} parameters;
			};
		}
	}
}
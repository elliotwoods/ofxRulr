#pragma once

#include "Constants_Plugin_ArUco.h"
#include "ofxRulr/Nodes/Base.h"
#include "ofxOsc.h"

namespace ofxRulr {
	namespace Nodes {
		namespace ArUco {
			class PLUGIN_ARUCO_EXPORTS OSCRelay : public ofxRulr::Nodes::Base {
			public:
				OSCRelay();
				string getTypeName() const override;
				void init();
				void update();
				void serialize(nlohmann::json &);
				void deserialize(const nlohmann::json &);
				void populateInspector(ofxCvGui::InspectArguments &);
			protected:
				struct Parameters : ofParameterGroup {
					ofParameter<string> remoteAddress{ "Remote address", "localhost" };
					ofParameter<int> remotePort{ "Remote port", 5000 };
					PARAM_DECLARE("OSCRelay", remoteAddress, remotePort);
				} parameters;

				Parameters cachedParameters;

				shared_ptr<ofxOscSender> getSender() const;
				shared_ptr<ofxOscSender> tryMakeSender() const;

				void setSender(shared_ptr<ofxOscSender>);
				void invalidateSender();

				unique_ptr<ofxOscSender> sender;
				mutable mutex senderMutex;
			};
		}
	}
}
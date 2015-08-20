#pragma once

#include "ofxRulr/Nodes/DMX/Transmit.h"
#include "ofSerial.h"

namespace ofxRulr {
	namespace Nodes {
		namespace DMX {
			class EnttecUsbPro : public DMX::Transmit {
			public:
				EnttecUsbPro();
				void init();
				string getTypeName() const;

				void serialize(Json::Value &);
				void deserialize(const Json::Value &);
			protected:
				void connect();
				void disconnect();

				void sendUniverse(UniverseIndex, shared_ptr<Universe>) override;

				void populateInspector(ofxCvGui::ElementGroupPtr);

				shared_ptr<ofSerial> sender;

				ofParameter<string> portName;
			};
		}
	}
}
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
				void update();

				void serialize(Json::Value &);
				void deserialize(const Json::Value &);
			protected:
				void connect();
				void disconnect();

				void populateInspector(ofxCvGui::ElementGroupPtr);

				shared_ptr<ofSerial> sender;

				ofParameter<string> portName;
			};
		}
	}
}
#pragma once

#include "ThreadedProcessNode.h"
#include "UpdateTracking.h"
#include "ofxOsc.h"

namespace ofxRulr {
	namespace Nodes {
		namespace MoCap {
			class OSCRelay : public ThreadedProcessNode<UpdateTracking
			, UpdateTrackingFrame
			, void *> {
			public:
				OSCRelay();
				string getTypeName() const override;
				void init();
				void serialize(Json::Value &);
				void deserialize(const Json::Value &);
				void populateInspector(ofxCvGui::InspectArguments &);
			protected:
				struct : ofParameterGroup {
					ofParameter<string> remoteAddress{ "Remote address", "localhost" };
					ofParameter<int> remotePort{ "Remote port", 5000 };
					PARAM_DECLARE("OSCRelay", remoteAddress, remotePort);
				} parameters;

				void processFrame(shared_ptr<UpdateTrackingFrame> incomingFrame) override;

				shared_ptr<ofxOscSender> getSender() const;
				shared_ptr<ofxOscSender> tryMakeSender() const;

				void setSender(shared_ptr<ofxOscSender>);
				void invalidateSender();

				shared_ptr<ofxOscSender> sender;
				mutable mutex senderMutex;
			};
		}
	}
}
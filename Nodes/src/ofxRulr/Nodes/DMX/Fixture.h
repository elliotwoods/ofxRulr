#pragma once

#include "Base.h"

namespace ofxRulr {
	namespace Nodes {
		namespace DMX {
			class Fixture : public DMX::Base {
			public:
				class Channel {
				public:
					Channel();
					Channel(string name);
					Channel(string name, function<DMX::Value()> generateValue);
					ofParameter<float> value;
					ofParameter<bool> enabled; // e.g. when fixtures have variable number of channels. warning : disabled channels should be at the end of the chain
					function<DMX::Value()> generateValue;
				};
				Fixture();
				void init();
				virtual string getTypeName() const override;
				void update();

				void serialize(Json::Value &);
				void deserialize(const Json::Value &);

				const vector<shared_ptr<Channel>> & getChannels() const;
			protected:
				void populateInspector(ofxCvGui::ElementGroupPtr);

				ofParameter<DMX::ChannelIndex> channelIndex;
				ofParameter<DMX::UniverseIndex> universeIndex;
				vector<shared_ptr<Channel>> channels;
			};
		}
	}
}
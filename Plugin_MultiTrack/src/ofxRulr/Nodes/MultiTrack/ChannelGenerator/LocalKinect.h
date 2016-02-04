#pragma once

#include "ofxRulr/Nodes/Base.h"
#include "ofxRulr/Nodes/Data/Channels/Generator/Base.h"

namespace ofxRulr {
	namespace Nodes {
		namespace MultiTrack {
			namespace ChannelGenerator {
				class LocalKinect : public Nodes::Data::Channels::Generator::Base {
				public:
					LocalKinect();
					string getTypeName() const;

					void init();
					void update();

					void populateData(ofxRulr::Data::Channels::Channel &) override;
				protected:
					shared_ptr<ofxRulr::Data::Channels::Channel> nodesChannel;
				};
			}
		}
	}
}
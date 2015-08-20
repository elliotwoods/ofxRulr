#pragma once

#include "MovingHead.h"

namespace ofxRulr {
	namespace Nodes {
		namespace DMX {
			class Sharpy : public MovingHead {
			public:
				Sharpy();
				void init();
				string getTypeName() const;

				void serialize(Json::Value &);
				void deserialize(const Json::Value &);
				void populateInspector(ofxCvGui::ElementGroupPtr);

			protected:
				void updateVectorChannelsEnabled();
				vector<shared_ptr<Channel>> vectorChannels;
				ofParameter<bool> vectorChannelsEnabled;
			};
		}
	}
}
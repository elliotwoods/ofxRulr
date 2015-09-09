#pragma once

#include "MovingHead.h"

namespace ofxRulr {
	namespace Nodes {
		namespace DMX {
			class Sharpy : public MovingHead {
			public:
				Sharpy();
				void init();
				void update();

				string getTypeName() const;

				void serialize(Json::Value &);
				void deserialize(const Json::Value &);
				void populateInspector(ofxCvGui::ElementGroupPtr);

				void setColorIndex(int);
				void reboot();
				void lampOn(bool on = true);
			protected:
				void updateVectorChannelsEnabled();
				vector<shared_ptr<Channel>> vectorChannels;
				ofParameter<bool> vectorChannelsEnabled;
			
				struct {
					bool rebooting;
					float rebootBeginTime;
				} rebootState;

				struct {
					bool changing;
					float beginTime;
					int value;
				} lampState;
			};
		}
	}
}
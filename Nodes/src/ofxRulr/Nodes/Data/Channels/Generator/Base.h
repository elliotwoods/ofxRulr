#pragma once

#include "ofxRulr/Nodes/Base.h"
#include "ofxRulr/Data/Channels/Channel.h"
#include "../Database.h"

namespace ofxRulr {
	namespace Nodes {
		namespace Data {
			namespace Channels {
				namespace Generator {
					using namespace ofxRulr::Data::Channels;

					class Base : public Nodes::Base, public enable_shared_from_this<Base> {
					public:
						Base();
						virtual string getTypeName() const override;

						void init();
						void populateInspector(ofxCvGui::InspectArguments &);
						void serialize(Json::Value &);
						void deserialize(const Json::Value &);

						const Address & getAddress() const;

						virtual void populateData(Channel &) { }
					protected:
						void connectDatabase(shared_ptr<Database>);
						void disconnectDatabase(shared_ptr<Database>);

						void addressParameterCallback(string &);
						ofParameter<string> addressParameter;
						Address address;
					};
				}
			}
		}
	}
}
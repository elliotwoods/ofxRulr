#pragma once

#include "ofxRulr/Nodes/Base.h"
#include "ofxRulr/Nodes/Graph/Patch.h"

#include "NodeHost.h"

namespace ofxRulr {
	namespace Graph {
		namespace Editor {
			using namespace ofxRulr::Nodes::Graph;
			class ExposeInputs : public Nodes::Base {
			public:
				ExposeInputs();
				void init();
				string getTypeName() const override;
				void setHost(shared_ptr<Patch>);
			protected:
				weak_ptr<Patch> host;
			};
		}
	}
}
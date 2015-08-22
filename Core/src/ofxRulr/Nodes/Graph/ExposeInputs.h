#pragma once

#include "ofxRulr/Nodes/Base.h"
#include "Patch.h"

namespace ofxRulr {
	namespace Nodes {
		namespace Graph {
			class ExposeInputs : public Nodes::Base {
			public:
				ExposeInputs();
				string getTypeName() const override;
				void init();

				void setHost(shared_ptr<Patch>);
			protected:
				weak_ptr<Patch> host;
			};
		}
	}
}
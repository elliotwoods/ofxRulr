#pragma once

#include "ofxRulr/Nodes/Base.h"

namespace ofxRulr {
	namespace Nodes {
		namespace DMX {
			class AimMovingHeadAt : public Nodes::Base {
			public:
				AimMovingHeadAt();
				void init();
				string getTypeName() const override;
				void update();
			};
		}
	}
}
#pragma once

#include "ofxRulr/Nodes/Base.h"

namespace ofxRulr {
	namespace Nodes {
		namespace DMX {
			typedef uint16_t ChannelIndex;
			typedef uint8_t Value;
			typedef uint16_t UniverseIndex;

			class Base : public virtual Nodes::Base {
			public:
				Base();
				void init();
			};
		}
	}
}
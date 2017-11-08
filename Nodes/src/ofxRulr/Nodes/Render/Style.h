#pragma once

#include "ofxRulr.h"

namespace ofxRulr {
	namespace Nodes {
		namespace Render {
			class Style : virtual public Base {
			public:
				Style();
				string getTypeName() const override;
				void init();
				void begin();
				void end();
			protected:
				virtual void customBegin() { }
				virtual void customEnd() { }
			};
		}
	}
}
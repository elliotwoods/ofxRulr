#pragma once

#include "ofxRulr/Nodes/Base.h"

#include "ofxCvGui/Panels/Widgets.h"

namespace ofxRulr {
	namespace Nodes {
		namespace Application {
			class Debugger : public Base {
			public:
				Debugger();
				string getTypeName() const override;
				void init();
			protected:
			};
		}
	}
}
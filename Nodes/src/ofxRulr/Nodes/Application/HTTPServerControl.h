#pragma once

#include "ofxRulr/Nodes/Base.h"

namespace ofxRulr {
	namespace Nodes {
		namespace Application {
			class HTTPServerControl : public Nodes::Base {
			public:
				HTTPServerControl();
				virtual ~HTTPServerControl();
				string getTypeName() const override;
				void init();
				void update();
			protected:
				struct : ofParameterGroup {
					ofParameter<bool> run { "Run", false};
					PARAM_DECLARE("HTTPServerControl", run)
				} parameters;
			};
		}
	}
}
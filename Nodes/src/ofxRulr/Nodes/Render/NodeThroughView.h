#pragma once

#include "ofxRulr/Nodes/Base.h"

namespace ofxRulr {
	namespace Nodes {
		namespace Render {
			class NodeThroughView : public Nodes::Base {
			public:
				NodeThroughView();
				string getTypeName() const override;
				void init();
			protected:
				void drawOnVideoOutput(const ofRectangle &);

				struct : ofParameterGroup {

				};
			};
		}
	}
}
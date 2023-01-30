#pragma once

#include "ofxRulr/Nodes/Base.h"

namespace ofxRulr {
	namespace Nodes {
		namespace Render {
			class WorldThroughView : public Nodes::Base {
			public:
				WorldThroughView();
				string getTypeName() const override;
				void init();
			protected:
				void drawOnVideoOutput(const ofRectangle&);

				struct : ofParameterGroup {

				};
			};
		}
	}
}
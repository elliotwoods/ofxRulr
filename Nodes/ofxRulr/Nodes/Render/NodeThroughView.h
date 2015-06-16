#pragma once

#include "ofxRulr/Graph/Node.h"

namespace ofxRulr {
	namespace Render {
		class NodeThroughView : public Graph::Node {
		public:
			NodeThroughView();
			string getTypeName() const override;
			void init();
		protected:
			void drawOnVideoOutput(const ofRectangle &);
		};
	}
}
#pragma once

#include "../Graph/Node.h"

namespace ofxDigitalEmulsion {
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
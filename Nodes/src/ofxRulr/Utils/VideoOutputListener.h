#pragma once

#include "ofxRulr/Nodes/System/VideoOutput.h"
#include "ofxRulr/Graph/Pin.h"

namespace ofxRulr {
	namespace Utils {
		class VideoOutputListener {
		public:
			VideoOutputListener(shared_ptr<Graph::Pin<Nodes::System::VideoOutput>>, function<void(const ofRectangle &)>);
			~VideoOutputListener();
		protected:
			function<void(const ofRectangle &)> action;
			shared_ptr<Graph::Pin<Nodes::System::VideoOutput>> pin;
		};
	}
}
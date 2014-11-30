#pragma once

#include "ofxDigitalEmulsion/Graph/Node.h"
#include "Frame.h"

namespace ofxDigitalEmulsion {
	namespace MotionCapture {
		///All nodes which represent Motion Capture devices (Vicon-like) should inherit this node
		class Base : public Graph::Node {
		public:
			virtual string getTypeName() const override {
				return "Tracker::Base";
			}

			virtual shared_ptr<Frame> getCurrentFrame() = 0;
		};
	}
}
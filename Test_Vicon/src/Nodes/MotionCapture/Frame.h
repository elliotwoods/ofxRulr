#pragma once

#include "ofxDigitalEmulsion/Graph/Node.h"

namespace ofxDigitalEmulsion {
	namespace MotionCapture {
		///This represents a frame of data from the 
		struct Frame {
			vector<ofVec3f> points;
		};
	}
}
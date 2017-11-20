#pragma once

#include "ofxRulr/Nodes/Base.h"
#include <opencv2/aruco.hpp>

namespace ofxRulr {
	namespace Nodes {
		namespace ArUco {
			class MarkerMapPoseTracker : public Nodes::Base {
			public:
				MarkerMapPoseTracker();
				string getTypeName() const override;
				void init();
				void update();
			protected:
				struct : ofParameterGroup {
					ofParameter<bool> enabled{ "Enabled", true };
					PARAM_DECLARE("MarkerMapPoseTracker", enabled);
				} parameters;
				aruco::MarkerMapPoseTracker markerMapPoseTracker;
			};
		}
	}
}
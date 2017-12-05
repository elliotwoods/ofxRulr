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
				void track();
				void populateInspector(ofxCvGui::InspectArguments &);
			protected:
				struct : ofParameterGroup {
					ofParameter<bool> onNewFrame{ "On new frame", true };
					PARAM_DECLARE("MarkerMapPoseTracker", onNewFrame);
				} parameters;
				aruco::MarkerMapPoseTracker markerMapPoseTracker;
			};
		}
	}
}
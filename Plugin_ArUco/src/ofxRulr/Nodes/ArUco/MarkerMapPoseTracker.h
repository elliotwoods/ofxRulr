#pragma once

#include "ofxRulr/Nodes/Base.h"
#include <opencv2/aruco.hpp>

namespace ofxRulr {
	namespace Nodes {
		namespace ArUco {
			class MarkerMapPoseTracker : public Nodes::Base {
			public:
				MAKE_ENUM(Method
					, (Tracker, solvePnP, RANSAC)
					, ("Tracker", "solvePnP", "RANSAC"));

				MarkerMapPoseTracker();
				string getTypeName() const override;
				void init();
				void update();
				void track();
				void populateInspector(ofxCvGui::InspectArguments &);
			protected:
				struct : ofParameterGroup {
					ofParameter<bool> onNewFrame{ "On new frame", true };
					ofParameter<int> minMarkerCount{ "Minimum marker count", 3 };
					ofParameter<Method> method{ "Method", Method::solvePnP };
					PARAM_DECLARE("MarkerMapPoseTracker", onNewFrame, minMarkerCount, method);
				} parameters;
				aruco::MarkerMapPoseTracker markerMapPoseTracker;
			};
		}
	}
}
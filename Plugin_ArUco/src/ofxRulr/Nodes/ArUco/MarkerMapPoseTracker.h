#pragma once

#include "Constants_Plugin_ArUco.h"
#include "ofxRulr/Nodes/Base.h"
#include <opencv2/aruco.hpp>

namespace ofxRulr {
	namespace Nodes {
		namespace ArUco {
			class PLUGIN_ARUCO_EXPORTS MarkerMapPoseTracker : public Nodes::Base {
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
					ofParameter<bool> useExtrinsicGuess{ "Use extrinsic guess", false  };
					PARAM_DECLARE("MarkerMapPoseTracker", onNewFrame, minMarkerCount, method, useExtrinsicGuess);
				} parameters;
				aruco::MarkerMapPoseTracker markerMapPoseTracker;
			};
		}
	}
}
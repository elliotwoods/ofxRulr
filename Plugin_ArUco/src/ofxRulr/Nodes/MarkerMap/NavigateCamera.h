#pragma once

#include "Constants_Plugin_ArUco.h"
#include "ofxRulr/Nodes/Base.h"
#include <opencv2/aruco.hpp>

namespace ofxRulr {
	namespace Nodes {
		namespace MarkerMap {
			class PLUGIN_ARUCO_EXPORTS NavigateCamera : public Nodes::Base {
			public:
				NavigateCamera();
				string getTypeName() const override;
				void init();
				void update();
				void track(const cv::Mat& image);
				void populateInspector(ofxCvGui::InspectArguments&);
			protected:
				struct : ofParameterGroup {
					ofParameter<bool> onNewFrame{ "On new frame", true };
					ofParameter<int> minMarkerCount{ "Minimum marker count", 3 };

					struct : ofParameterGroup {
						ofParameter<bool> enabled{ "Enabled", true };
						ofParameter<int> maxIterations{ "Max iterations", 100 };
						ofParameter<float> reprojectionError{ "Reprojection Error", 2.0, 0.0, 100.0};
						PARAM_DECLARE("RANSAC", enabled, maxIterations, reprojectionError);
					} ransac;

					ofParameter<bool> useExtrinsicGuess{ "Use extrinsic guess", false };
					PARAM_DECLARE("MarkerMapPoseTracker", onNewFrame, minMarkerCount, ransac, useExtrinsicGuess);
				} parameters;
			};
		}
	}
}
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
				void drawWorldStage();
				void track(const cv::Mat& image);
				void populateInspector(ofxCvGui::InspectArguments&);
			protected:
				struct : ofParameterGroup {
					ofParameter<bool> onNewFrame{ "On new frame", true };
					ofParameter<int> minMarkerCount{ "Minimum marker count", 3 };
					
					struct : ofParameterGroup {
						ofParameter<bool> enabled{ "Enabled", true };
						ofParameter<float> searchRange{ "Search range", 2.0f, 1.0f, 10.0f };
						PARAM_DECLARE("Enabled", enabled, searchRange);
					} findMissingMarkers;

					struct : ofParameterGroup {
						ofParameter<bool> enabled{ "Enabled", false };
						ofParameter<int> maxIterations{ "Max iterations", 100 };
						ofParameter<float> reprojectionError{ "Reprojection Error", 2.0, 0.0, 100.0};
						PARAM_DECLARE("RANSAC", enabled, maxIterations, reprojectionError);
					} ransac;

					struct : ofParameterGroup {
						ofParameter<bool> speakCount{ "Speak count", true };
						PARAM_DECLARE("Debug", speakCount)
					} debug;

					ofParameter<bool> useExtrinsicGuess{ "Use extrinsic guess", false };
					PARAM_DECLARE("MarkerMapPoseTracker"
						, onNewFrame
						, minMarkerCount
						, findMissingMarkers
						, ransac
						, useExtrinsicGuess
					, debug);
				} parameters;

				vector<ofxRay::Ray> cameraRays;
			};
		}
	}
}
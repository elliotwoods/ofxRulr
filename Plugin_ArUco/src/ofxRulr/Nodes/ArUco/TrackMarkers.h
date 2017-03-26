#pragma once

#include "ofxRulr/Nodes/Base.h"
#include <aruco.h>

namespace ofxRulr {
	namespace Nodes {
		namespace ArUco {
			class TrackMarkers : public Nodes::Base {
			public:
				struct TrackedMarker {
					int index;
					ofMatrix4x4 transform;
					float markerLength;
					cv::Mat rotation, translation;
					ofVec2f cornersInImage[4];
					ofVec3f cornersInObjectSpace[4];
				};

				TrackMarkers();
				string getTypeName() const override;
				void init();
				void update();
				void drawWorld();
				ofxCvGui::PanelPtr getPanel() override;

				const multimap<int, unique_ptr<TrackedMarker>> & getTrackedMarkers() const;
				const vector<aruco::Marker> getRawMarkers() const;
			protected:
				ofFbo previewMask;
				ofFbo previewComposite;

				vector<aruco::Marker> rawMarkers;
				multimap<int, unique_ptr<TrackedMarker>> trackedMarkers;
				ofxCvGui::PanelPtr panel;

				ofMesh previewPlane;
			};
		}
	}
}
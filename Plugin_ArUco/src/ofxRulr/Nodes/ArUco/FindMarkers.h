#pragma once

#include "Constants_Plugin_ArUco.h"
#include "ofxRulr/Nodes/Base.h"
#include <aruco/aruco.h>

namespace ofxRulr {
	namespace Nodes {
		namespace ArUco {
			class FindMarkers : public Nodes::Base {
			public:
				struct TrackedMarker {
					int ID;
					ofMatrix4x4 transform;
					float markerLength;
					cv::Mat rotation, translation;
					ofVec2f cornersInImage[4];
					ofVec3f cornersInObjectSpace[4];
				};

				FindMarkers();
				string getTypeName() const override;
				void init();
				void update();
				void drawWorldStage();
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

				struct : ofParameterGroup {
					struct : ofParameterGroup {
						struct : ofParameterGroup {
							ofParameter<bool> enabled{ "Enabled", false };
							ofParameter<bool> onlySaveOnFind{ "Only save on find", false };
							ofParameter<filesystem::path> folder{ "Save folder", "" };
							PARAM_DECLARE("Save", enabled, onlySaveOnFind, folder);
						} save;
						ofParameter<bool> speakCount{ "Speak count", true };
						PARAM_DECLARE("Tethered options", save, speakCount);
					} tetheredOptions;
					PARAM_DECLARE("FindMarkers", tetheredOptions);
				} parameters;
			};
		}
	}
}
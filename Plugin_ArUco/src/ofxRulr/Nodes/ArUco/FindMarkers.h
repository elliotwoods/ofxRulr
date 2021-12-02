#pragma once

#include "Constants_Plugin_ArUco.h"
#include "ofxRulr/Nodes/Base.h"
#include <aruco/aruco.h>

namespace ofxRulr {
	namespace Nodes {
		namespace ArUco {
			MAKE_ENUM(WhenToSave
				, (Never, Success, Always)
				, ("Never", "Success", "Always"));

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
				void populateInspector(ofxCvGui::InspectArguments&);

				ofxCvGui::PanelPtr getPanel() override;

				void detect();

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
						ofParameter<WhenActive> processWhen{ "Process when", WhenActive::Always };
						ofParameter<bool> speakCount{ "Speak count", true };

						PARAM_DECLARE("Detection", processWhen, speakCount)
					} detection;
					
					struct : ofParameterGroup {
						ofParameter<WhenToSave> whenToSave{ "When to save", WhenToSave::Never };
						ofParameter<filesystem::path> folder{ "Save folder", "" };
						PARAM_DECLARE("Save", whenToSave, folder);
					} save;

					ofParameter<WhenActive> drawLabels{ "Draw labels", WhenActive::Selected };
					PARAM_DECLARE("FindMarkers", detection, save, drawLabels);
				} parameters;
			};
		}
	}
}
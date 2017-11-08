#pragma once

#include "ofxRulr/Nodes/Base.h"
#include <aruco.h>

namespace ofxRulr {
	namespace Nodes {
		namespace ArUco {
			class MarkerMap : public Nodes::Base {
			public:
				MarkerMap();
				string getTypeName() const override;
				void init();
				void update();
				void drawWorldStage();

				void populateInspector(ofxCvGui::InspectArguments &);
				void serialize(Json::Value &);
				void deserialize(const Json::Value &);

				void clear();
			protected:
				struct : ofParameterGroup {
					ofParameter<string> filename{ "Filename", "" };
					ofParameter<bool> updateCamera{ "Update camera", true };
					PARAM_DECLARE("MarkerMap", filename, updateCamera);
				} parameters;

				aruco::MarkerMapPoseTracker markerMapPoseTracker;
				aruco::MarkerMap markerMap;
				string loadedMarkerMap = "";
			};
		}
	}
}
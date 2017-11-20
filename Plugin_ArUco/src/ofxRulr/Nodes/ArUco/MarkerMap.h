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
				void drawWorldStage();

				void populateInspector(ofxCvGui::InspectArguments &);
				void serialize(Json::Value &);
				void deserialize(const Json::Value &);

				shared_ptr<aruco::MarkerMap> getMarkerMap();

				void clear();
			protected:
				shared_ptr<aruco::MarkerMap> markerMap;
			};
		}
	}
}
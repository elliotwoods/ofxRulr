#ifdef OFXMULTITRACK_TCP

#pragma once

#include "ofxRulr/Nodes/Item/IDepthCamera.h"
#include "ofxMultiTrack.h"

namespace ofxRulr {
	namespace Nodes {
		namespace MultiTrack {
			namespace Test {
				class FindMarker : public Nodes::Base {
				public:
					FindMarker();
					string getTypeName() const override;
					void init();
					void update();

					ofxCvGui::PanelPtr getPanel() override;

					void drawWorld();

					void populateInspector(ofxCvGui::InspectArguments &);
					void serialize(Json::Value &);
					void deserialize(const Json::Value &);
				protected:
					ofxCvGui::PanelPtr panel;
					struct : ofParameterGroup {
						ofParameter<bool> enabled{ "Enabled", true };
						ofParameter<float> threshold{ "Threshold", 100, 0, 255 };
						ofParameter<float> minimumArea{ "Minimum area", 5 * 5 };
						ofParameter<float> markerPadding{ "Marker padding [%]", 50, 0, 100 };
						PARAM_DECLARE("FindMarker", enabled, threshold, minimumArea);
					} parameters;

					ofTexture infrared;
					ofImage threshold;

					struct Marker {
						ofPolyline outline;

						ofVec2f center;
						float radius;

						ofVec3f position;
					};
					vector<Marker> markers;
				};
			}
		}
	}
}

#endif // OFXMULTITRACK_TCP
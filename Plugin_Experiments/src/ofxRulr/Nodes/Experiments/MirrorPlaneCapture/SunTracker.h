#pragma once

#include "pch_Plugin_Experiments.h"

namespace ofxRulr {
	namespace Nodes {
		namespace Experiments {
			namespace MirrorPlaneCapture {
				class SunTracker : public Item::RigidBody {
				public:
					SunTracker();
					string getTypeName() const override;

					void init();
					void update();
					void populateInspector(ofxCvGui::InspectArguments&);

					void drawObject();

					glm::vec2 getAzimuthAltitude(const chrono::system_clock::time_point&) const;
					glm::vec3 getSolarVectorObjectSpace(const chrono::system_clock::time_point&) const;
					glm::vec3 getSolarVectorWorldSpace(const chrono::system_clock::time_point&) const;
				protected:
					struct : ofParameterGroup {
						struct : ofParameterGroup {
							ofParameter<float> latitude {"Latitude", 37.5789701, -90, 90};
							ofParameter<float> longitude {"Longitude", 126.9805752, -180, 180};
							ofParameter<float> atmosphericPressure {"Atmospheric pressure (kPa)", 101};
							ofParameter<float> temperature {"Temperature (C)", 25};
							PARAM_DECLARE("Location", latitude, longitude, atmosphericPressure, temperature);
						} location;

						struct : ofParameterGroup {
							ofParameter<bool> renderInternal{"Render internal", true};
							ofParameter<float> sphereRadius{"Sphere radius", 1.0f, 0.0f, 10.0f};
							PARAM_DECLARE("Draw", renderInternal, sphereRadius);
						} draw;
						PARAM_DECLARE("SunTracker", location, draw);
					};

					ofLight light;
				};
			}
		}
	}
}
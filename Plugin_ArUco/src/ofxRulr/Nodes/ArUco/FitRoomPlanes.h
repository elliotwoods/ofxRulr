#pragma once

#include "ofxRulr/Nodes/Base.h"
#include "ofxRulr/Utils/CaptureSet.h"

#include <aruco.h>

namespace ofxRulr {
	namespace Nodes {
		namespace ArUco {
			class FitRoomPlanes : public Nodes::Base {
			public:
				enum class Plane : int {
					X, Y, Z
				};

				class Capture : public Utils::AbstractCaptureSet::BaseCapture {
				public:
					Capture();
					string getDisplayString() const override;

					ofParameter<int> markerIndex{ "Marker index", 0 };
					ofParameter<int> plane{ "Plane", (int) Plane::X };
				protected:
					ofxCvGui::ElementPtr getDataDisplay() override;
					void serialize(Json::Value &);
					void deserialize(const Json::Value &);
				};

				FitRoomPlanes();
				string getTypeName() const override;
				void init();
				void update();
				void drawWorld();

				void populateInspector(ofxCvGui::InspectArguments &);
				void serialize(Json::Value &);
				void deserialize(const Json::Value &);
			protected:
				Utils::CaptureSet<Capture> captureSet;
			};
		}
	}
}
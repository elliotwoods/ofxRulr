#pragma once

#include "ofMain.h"
#include "ofxCvGui.h"
#include "ofxRulr.h"

namespace ofxRulr {
	namespace Nodes {
		namespace AppTester {
			class Project : public ofxCvGui::Element {
			public:
				enum Platform {
					Win32,
					x64
				};

				enum Configuration {
					Release,
					Debug
				};

				enum LocationType {
					Addons = 0,
					Apps
				};

				Project();
				Project(const filesystem::path & path, LocationType, const filesystem::path & relativePath);
				
				LocationType getLocationType() const;

				void build();

				static ofParameter<string> & getMSBuildLocation();

				void serialize(Json::Value &);
				void deserialize(const Json::Value &);
			protected:
				filesystem::path path;
				LocationType locationType;
				filesystem::path relativePath;


				bool ignore = false;
				set<pair<Platform, Configuration>> configurations;

				ofxCvGui::ElementGroupPtr elements;
			};
		}
	}
}
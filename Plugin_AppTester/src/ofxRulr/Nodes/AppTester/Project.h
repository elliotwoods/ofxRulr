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

				struct BuildConfiguration {
					BuildConfiguration(Platform, Configuration);
					Platform platform;
					Configuration configuration;
					vector<string> warnings;
					vector<string> errors;
					bool enabled = true;
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

				static string toString(Platform);
				static string toString(Configuration);
			protected:
				void rebuildGui();

				filesystem::path path;
				LocationType locationType;
				filesystem::path relativePath;

				bool ignore = false;
				vector<shared_ptr<BuildConfiguration>> buildConfigurations;

				ofxCvGui::ElementGroupPtr elements;
				ofxCvGui::ElementGroupPtr buildConfigurationElements;
				ofxCvGui::ElementPtr buildButton;
			};
		}
	}
}
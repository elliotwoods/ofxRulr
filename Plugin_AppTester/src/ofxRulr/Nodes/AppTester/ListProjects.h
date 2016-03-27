#pragma once

#include "ofxRulr.h"
#include "ofxCvGui.h"

#include "Project.h"

namespace ofxRulr {
	namespace Nodes {
		namespace AppTester {
			class ListProjects : public Nodes::Base {
			public:
				ListProjects();
				string getTypeName() const;

				void init();
				void populateInspector(ofxCvGui::InspectArguments &);
				void serialize(Json::Value &);
				void deserialize(const Json::Value &);

				ofxCvGui::PanelPtr getPanel();

				void refresh();
				void clear();
			protected:
				void refreshDirectory(const filesystem::path & directory, const filesystem::path & relativePath, Project::LocationType);
				void rebuildView();

				shared_ptr<ofxCvGui::Panels::Widgets> view;

				ofParameter<string> openFrameworksFolder;
				ofParameter<bool> includeAddons;
				ofParameter<bool> includeApps;

				map<filesystem::path, shared_ptr<Project>> projects;
			};
		}
	}
}
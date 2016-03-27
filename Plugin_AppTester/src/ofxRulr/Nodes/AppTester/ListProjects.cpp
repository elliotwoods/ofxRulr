#include "pch_AppTester.h"
#include "ListProjects.h"

using namespace ofxCvGui;

namespace ofxRulr {
	namespace Nodes {
		namespace AppTester {
			//----------
			ListProjects::ListProjects() {
				RULR_NODE_INIT_LISTENER;
			}

			//----------
			string ListProjects::getTypeName() const {
				return "AppTester::ListProjects";
			}

			//----------
			void ListProjects::init() {
				RULR_NODE_INSPECTOR_LISTENER;
				RULR_NODE_SERIALIZATION_LISTENERS;

				this->view = ofxCvGui::Panels::makeWidgets();

				this->openFrameworksFolder.set("openFrameworks folder", "e:\\openFrameworks");

				this->rebuildView();
			}

			//----------
			void ListProjects::populateInspector(InspectArguments & inspectArguments) {
				auto inspector = inspectArguments.inspector;
				inspector->addEditableValue(Project::getMSBuildLocation());
			}

			//----------
			void ListProjects::serialize(Json::Value & json) {
				json << this->openFrameworksFolder;
				json << this->includeAddons;
				json << this->includeApps;

				auto & jsonProjects = json["projects"];
				for (auto project : projects) {
					project.second->serialize(jsonProjects[project.first.string()]);
				}
			}

			//----------
			void ListProjects::deserialize(const Json::Value & json) {
				json >> this->openFrameworksFolder;
				json >> this->includeAddons;
				json >> this->includeApps;

				this->projects.clear();
				const auto & jsonProjects = json["projects"];
				auto projectPaths = jsonProjects.getMemberNames();
				for (const auto & projectPath : projectPaths) {
					auto project = make_shared<Project>();
					project->deserialize(jsonProjects[projectPath]);
					this->projects.insert(make_pair(filesystem::path(projectPath), project));
				}

				this->rebuildView();
			}

			//----------
			ofxCvGui::PanelPtr ListProjects::getPanel() {
				return this->view;
			}

			namespace fs = std::filesystem;

			//----------
			void ListProjects::refresh() {
				ofxCvGui::Utils::drawProcessingNotice("Finding projects...");

				try {
					auto ofRoot = fs::path(this->openFrameworksFolder.get());
					if (!filesystem::exists(ofRoot) ||
						!filesystem::is_directory(ofRoot)) {
						throw(Exception(ofRoot.string() + " does not exist is or not a directory"));
					}

					auto addonsDirectory = ofRoot / "addons";
					auto appsDirectory = ofRoot / "apps";

					auto oldProjects = this->projects;
					this->projects.clear();

					this->refreshDirectory(addonsDirectory, fs::path(), Project::LocationType::Addons);
					this->refreshDirectory(appsDirectory, fs::path(), Project::LocationType::Apps);

					//if anything matches projects we used to have, keep those instances from the old list (e.g. with their properties)
					for (auto project : this->projects) {
						auto findInOldList = oldProjects.find(project.first);
						if (findInOldList != oldProjects.end()) {
							project.second = findInOldList->second;
						}
					}
				}
				RULR_CATCH_ALL_TO_ALERT;

				this->rebuildView();
			}

			//----------
			void ListProjects::clear() {
				this->projects.clear();
				this->rebuildView();
			}

			//----------
			void ListProjects::refreshDirectory(const fs::path & directory, const fs::path & relativePath, Project::LocationType locationType) {
				for (fs::directory_iterator dir(directory);
				dir != fs::directory_iterator(); ++dir
					) {
					auto & path = dir->path();

					//ignore git folder
					if (path.filename().string() == ".git") {
						continue;
					}

					if (fs::is_directory(path)) {
						//don't go deeper than 2
						size_t length = 0;
						for (auto level : relativePath) {
							length++;
						}
						if (length < 2) {
							this->refreshDirectory(path, relativePath / path.filename(), locationType);
						}
					}
					else {
						auto extension = ofToLower(path.extension().string());
						if (extension == ".sln") {
							this->projects.insert(make_pair(path, make_shared<Project>(path, locationType, relativePath)));
							ofxCvGui::Utils::drawProcessingNotice("Found " + ofToString(this->projects.size()) + " projects...");
						}
					}
				}
			}

			//----------
			void ListProjects::rebuildView() {
				this->view->clear();

				this->view->addEditableValue(this->openFrameworksFolder);

				this->view->addLiveValue<size_t>("Projects found", [this]() {
					return this->projects.size();
				});
				this->view->addButton("Refresh", [this]() {
					try {
						this->refresh();
					}
					RULR_CATCH_ALL_TO_ALERT;
				});
				this->view->addButton("Clear", [this]() {
					try {
						this->clear();
					}
					RULR_CATCH_ALL_TO_ALERT;
				});

				//addons
				this->view->addTitle("Addons");
				for (auto projectIt : this->projects) {
					auto project = projectIt.second;
					if (project->getLocationType() == Project::LocationType::Addons) {
						this->view->add(project);
					}
				}

				//apps
				this->view->addTitle("Apps");
				for (auto projectIt : this->projects) {
					auto project = projectIt.second;
					if (project->getLocationType() == Project::LocationType::Apps) {
						this->view->add(project);
					}
				}
			}
		}
	}
}
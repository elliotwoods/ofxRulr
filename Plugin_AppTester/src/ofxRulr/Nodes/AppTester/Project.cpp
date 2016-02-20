#include "pch_AppTester.h"
#include "Project.h"

using namespace ofxCvGui;

namespace ofxRulr {
	namespace Nodes {
		namespace AppTester{
			//----------
			Project::Project() {
				this->onDraw += [this](DrawArguments & args) {
					//background
					ofPushStyle();
					{
						switch (this->locationType) {
						case Addons:
							ofSetColor(100, 100, 150);
							break;
						case Apps:
							ofSetColor(100, 150, 100);
							break;
						}
						ofDrawRectangle(args.localBounds);
					}
					ofPopStyle();

					//title
					ofxCvGui::Utils::drawText(this->relativePath.string(), 90, 5, false);
				};

				this->elements = make_shared<ofxCvGui::ElementGroup>();
				this->elements->addListenersToParent(this, true);

				auto buildButton = ofxCvGui::Widgets::makeButton("Build", [this]() {
					this->build();
				});
				this->elements->add(buildButton);

				this->onBoundsChange += [this, buildButton](BoundsChangeArguments & args) {
					buildButton->setBounds(ofRectangle(0, 0, 80, args.localBounds.height));
				};

				this->setHeight(50.0f);
			}

			//----------
			Project::Project(const filesystem::path & path, LocationType locationType, const filesystem::path & relativePath) :
			Project() {
				this->path = path;
				this->locationType = locationType;
				this->relativePath = relativePath;

				this->configurations.insert(make_pair(Platform::Win32, Configuration::Debug));
				this->configurations.insert(make_pair(Platform::Win32, Configuration::Release));
				this->configurations.insert(make_pair(Platform::x64, Configuration::Debug));
				this->configurations.insert(make_pair(Platform::x64, Configuration::Release));
			}

			//----------
			Project::LocationType Project::getLocationType() const {
				return this->locationType;
			}

			//----------
			void Project::build() {
				auto buildExe = Project::getMSBuildLocation().get();

				for (auto configuration : this->configurations) {
					stringstream execString;
					execString << "cmd /S /c \"" << buildExe << "\" \"" << this->path.string() << "\"";
					
					switch (configuration.first) {
					case Win32:
						execString << " /pPlatform=Win32";
						break;
					case x64:
						execString << " /pPlatform=x64";
						break;
					}

					switch (configuration.second) {
					case Debug:
						execString << " /p:Configuration=Debug";
						break;
					case Release:
						execString << " /p:Configuration=Release";
						break;
					}
					
					cout << execString << endl;

					ofSystem(execString.str().c_str());
				}
				

				system(buildExe.c_str());
			}

			//----------
			ofParameter<string> & Project::getMSBuildLocation() {
				static ofParameter<string> MSBuildLocation("MSBuild Location", "C:\\Program Files (x86)\\MSBuild\\14.0\\Bin\\MSBuild.exe");
				return MSBuildLocation;
			}

			//----------
			void Project::serialize(Json::Value & json) {
				json["LocationType"] = this->locationType;
				json["relativePath"] = this->relativePath.string();

				auto & jsonConfigurations = json["configurations"];
				int i = 0;
				for (const auto & configuration : this->configurations) {
					auto & jsonConfiguration = jsonConfigurations[i++];
					jsonConfiguration["Platform"] = configuration.first;
					jsonConfiguration["Configuration"] = configuration.second;
				}
			}

			//----------
			void Project::deserialize(const Json::Value & json) {
				this->locationType = (LocationType) json["LocationType"].asInt();
				this->relativePath = filesystem::path(json["relativePath"].asString());

				this->configurations.clear();
				const auto & jsonConfigurations= json["configurations"];
				for (auto & jsonConfiguration : jsonConfigurations) {
					auto platform = (Platform) jsonConfiguration["Platform"].asInt();
					auto configuration = (Configuration) jsonConfiguration["Configuration"].asInt();
					this->configurations.insert(make_pair(platform, configuration));
				}
			}
		}
	}
}
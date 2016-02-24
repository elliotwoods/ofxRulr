#include "pch_AppTester.h"
#include "Project.h"

using namespace ofxCvGui;

namespace ofxRulr {
	namespace Nodes {
		namespace AppTester{
			//----------
			Project::BuildConfiguration::BuildConfiguration(Platform platform, Configuration configuration) {
				this->platform = platform;
				this->configuration = configuration;

				this->errors.push_back("Test error");
				this->warnings.push_back("Test error");
			}

			//----------
			Project::Project() {
				this->onDraw += [this](DrawArguments & args) {
					//side line
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
						ofDrawLine(args.localBounds.getTopRight(), args.localBounds.getBottomRight());
					}
					ofPopStyle();

					//title
					auto & font = ofxAssets::font(ofxCvGui::getDefaultTypeface(), 20);
					font.drawString(this->relativePath.string(), 5, 20);
				};

				this->elements = make_shared<ofxCvGui::ElementGroup>();
				this->elements->addListenersToParent(this, true);
				this->buildConfigurationElements = make_shared<ofxCvGui::ElementGroup>();
				this->buildConfigurationElements->addListenersToParent(this, true);
				this->rebuildGui();
			}

			//----------
			Project::Project(const filesystem::path & path, LocationType locationType, const filesystem::path & relativePath) :
			Project() {
				this->path = path;
				this->locationType = locationType;
				this->relativePath = relativePath;

				this->buildConfigurations.push_back(make_shared<BuildConfiguration>(Platform::Win32, Configuration::Debug));
				this->buildConfigurations.push_back(make_shared<BuildConfiguration>(Platform::Win32, Configuration::Release));
				this->buildConfigurations.push_back(make_shared<BuildConfiguration>(Platform::x64, Configuration::Debug));
				this->buildConfigurations.push_back(make_shared<BuildConfiguration>(Platform::x64, Configuration::Release));

				this->rebuildGui();
			}

			//----------
			Project::LocationType Project::getLocationType() const {
				return this->locationType;
			}

			//----------
			void Project::build() {
				auto buildExe = Project::getMSBuildLocation().get();

				for (auto buildConfiguration : this->buildConfigurations) {
					stringstream execString;
					execString << "cmd /c";
					execString << " \"" << buildExe << "\"";

					switch (buildConfiguration->platform) {
					case Win32:
						execString << " /p:Platform=Win32";
						break;
					case x64:
						execString << " /p:Platform=x64";
						break;
					}

					switch (buildConfiguration->configuration) {
					case Debug:
						execString << " /p:Configuration=Debug";
						break;
					case Release:
						execString << " /p:Configuration=Release";
						break;
					}

					execString << " /verbosity:Quiet";

					execString << " " << this->path.string();

					cout << execString << endl;

					ofxCvGui::Utils::drawProcessingNotice("Building [" + this->relativePath.string() + "] " + toString(buildConfiguration->platform) + "|" + toString(buildConfiguration->configuration));
					{
#ifdef TARGET_WIN32
						auto handle = _popen(execString.str().c_str(), "r");
#else
						auto handle = popen(execString.str().c_str(), "r");
#endif
						if (!handle) {
							throw(Exception("Failed to execute command [" + execString.str() + "]"));
						}

						auto charachter = fgetc(handle);
						while (charachter != EOF) {
							cout << (char) charachter;
							charachter = fgetc(handle);
						}
#ifdef TARGET_WIN32
						_pclose(handle);
#else
						pclose(handle);
#endif
					}
				}
			}

			//----------
			ofParameter<string> & Project::getMSBuildLocation() {
				static ofParameter<string> MSBuildLocation("MSBuild Location", "C:\\Program Files (x86)\\MSBuild\\14.0\\Bin\\MSBuild.exe");
				return MSBuildLocation;
			}

			//----------
			void Project::serialize(Json::Value & json) {
				json["LocationType"] = this->locationType;
				json["path"] = this->path.string();
				json["relativePath"] = this->relativePath.string();

				auto & jsonConfigurations = json["buildConfigurations"];
				int i = 0;
				for (const auto & buildConfiguration : this->buildConfigurations) {
					auto & jsonConfiguration = jsonConfigurations[i++];
					jsonConfiguration["platform"] = buildConfiguration->platform;
					jsonConfiguration["configuration"] = buildConfiguration->configuration;

					auto warningIndex = 0;
					for (auto warning : buildConfiguration->warnings) {
						jsonConfiguration["warnings"][warningIndex] = warning;
					}

					auto errorIndex = 0;
					for (auto error : buildConfiguration->errors) {
						jsonConfiguration["errors"][errorIndex] = error;
					}

					jsonConfiguration["enabled"] = buildConfiguration->enabled;
				}
			}

			//----------
			void Project::deserialize(const Json::Value & json) {
				this->locationType = (LocationType) json["LocationType"].asInt();
				this->path = filesystem::path(json["path"].asString());
				this->relativePath = filesystem::path(json["relativePath"].asString());

				this->buildConfigurations.clear();
				const auto & jsonConfigurations= json["buildConfigurations"];
				for (auto & jsonConfiguration : jsonConfigurations) {
					auto platform = (Platform) jsonConfiguration["platform"].asInt();
					auto configuration = (Configuration) jsonConfiguration["configuration"].asInt();
					auto buildConfiguration = make_shared<BuildConfiguration>(platform, configuration);
					buildConfiguration->enabled = jsonConfiguration["enabled"].asBool();

					for (const auto & jsonWarning : jsonConfiguration["warnings"]) {
						buildConfiguration->warnings.push_back(jsonWarning.asString());
					}
					for (const auto & jsonError : jsonConfiguration["errors"]) {
						buildConfiguration->errors.push_back(jsonError.asString());
					}

					this->buildConfigurations.push_back(buildConfiguration);
				}

				this->rebuildGui();
			}

			//----------
			string Project::toString(Platform platform) {
				switch (platform) {
				case Win32:
					return "Win32";
				case x64:
					return "x64";
				default:
					return "Unsupported";
				}
			}

			//----------
			string Project::toString(Configuration configuration) {
				switch (configuration) {
				case Release:
					return "Release";
				case Debug:
					return "Debug";
				default:
					return "Unsupported";
				}
			}

			//----------
			void Project::rebuildGui() {
				this->elements->clear();

				auto height = 40.0f;

				auto buildButton = ofxCvGui::Widgets::makeButton("Build", [this]() {
					this->build();
				});
				this->elements->add(buildButton);

				for (auto buildConfiguration : this->buildConfigurations) {
					auto element = ofxCvGui::makeElement();

					element->onDraw += [buildConfiguration](ofxCvGui::DrawArguments & args) {
						ofxCvGui::Utils::drawText(toString(buildConfiguration->platform), 0, 0, false);
						ofxCvGui::Utils::drawText(toString(buildConfiguration->configuration), 50, 0, false);

						if (!buildConfiguration->warnings.empty()) {
							ofxAssets::image("ofxRulr::AppTester::warning").draw(120, 1, 16, 16);
							ofxCvGui::Utils::drawText(ofToString(buildConfiguration->warnings.size()), 135, 0, false);
						}

						if (!buildConfiguration->errors.empty()) {
							ofxAssets::image("ofxRulr::AppTester::error").draw(180, 1, 16, 16);
							ofxCvGui::Utils::drawText(ofToString(buildConfiguration->errors.size()), 195, 0, false);
						}
					};
					element->setHeight(20.0f);
					this->buildConfigurationElements->add(element);
					height += 20.0f;
				}

				this->onBoundsChange += [this, buildButton](BoundsChangeArguments & args) {
					buildButton->setBounds(ofRectangle(0, 40, 80, args.localBounds.height - 40));
					auto y = 40.0f;
					for (auto buildConfigurationElement : this->buildConfigurationElements->getElements()) {
						auto bounds = buildConfigurationElement->getBounds();
						bounds.x = buildButton->getWidth();
						bounds.width = args.localBounds.width - bounds.x;
						bounds.y = y;
						buildConfigurationElement->setBounds(bounds);
						y += bounds.height;
					}
				};

				this->setHeight(height);
			}
		}
	}
}
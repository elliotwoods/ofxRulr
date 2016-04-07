#include "pch_RulrNodes.h"
#include "openFrameworks.h"

#include "ofxCvGui/Widgets/Button.h"
#include "ofxCvGui/Widgets/Toggle.h"
#include "ofxCvGui/Widgets/EditableValue.h"

using namespace ofxCvGui;

namespace ofxRulr {
	namespace Nodes {
		namespace Application {
			//----------
			openFrameworks::openFrameworks() {
				RULR_NODE_INIT_LISTENER;
			}

			//----------
			string openFrameworks::getTypeName() const {
				return "Application::openFrameworks";
			}

			//----------
			void openFrameworks::init() {
				RULR_NODE_UPDATE_LISTENER;
				RULR_NODE_INSPECTOR_LISTENER;

				this->vsync = true;  // There's no getter for this so assume it's true (default).
				this->image = &ofxAssets::image("ofxRulr::Nodes::Application::ofLogo");
				this->panel = ofxCvGui::Panels::makeImage(*this->image, "Logo");
			}

			//----------
			void openFrameworks::update() {

			}

			//----------
			void openFrameworks::populateInspector(ofxCvGui::InspectArguments & inspectArgs) {
				auto inspector = inspectArgs.inspector;

				inspector->add(new Widgets::LiveValue<string>("OF Version", []() {
					return ofGetVersionInfo();
				}));

				inspector->add(new Widgets::LiveValue<string>("GL Version", []() {
					const auto version = ofToString(ofGetGLRenderer()->getGLVersionMajor()) + "." + ofToString(ofGetGLRenderer()->getGLVersionMinor());
					return version;
				}));

				inspector->add(new Widgets::EditableValue<float>("Frame Rate", []() {
					return ofGetFrameRate();
				},
				[](string valueString) {
					const auto value = ofToFloat(valueString);
					if (value > 0) {
						ofSetFrameRate(value);
					}
				}));

				inspector->add(new Widgets::Toggle("Vertical Sync", [this]() {
					return this->vsync;
				},
				[this](bool value) {
					this->vsync = value;
					ofSetVerticalSync(this->vsync);
				}));
			}

			////----------
			//void Assets::serialize(Json::Value & json) {
			//	ofxRulr::Utils::Serializable::serialize(this->filter, json);
			//}

			////----------
			//void Assets::deserialize(const Json::Value & json) {
			//	ofxRulr::Utils::Serializable::deserialize(this->filter, json);
			//}

			//----------
			ofxCvGui::PanelPtr openFrameworks::getPanel() {
				return this->panel;
			}
		}
	}
}
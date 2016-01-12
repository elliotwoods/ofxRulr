#include "pch_RulrNodes.h"
#include "Application.h"

#include "ofAppRunner.h"

namespace ofxRulr {
	namespace Nodes {
		namespace Data {
			namespace Channels {
				namespace Generator {
					//----------
					Application::Application() {
					}

					//----------
					string Application::getTypeName() const {
						return "Data::Channels::Application";
					}

					//----------
					void Application::populateData(Channel & channel) {
						channel["fps"] = ofGetFrameRate();
						channel["frameIndex"] = ofGetFrameNum();
						channel["upTime"] = ofGetElapsedTimef();
					}
				}
			}
		}
	}
}
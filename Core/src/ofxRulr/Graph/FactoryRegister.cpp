#include "FactoryRegister.h"
#include "ofxRulr/Exception.h"
#include "ofSystemUtils.h"

OFXSINGLETON_DEFINE(ofxRulr::Graph::FactoryRegister);

namespace ofxRulr {
	namespace Graph {
#pragma mark FactoryRegister
		//----------
		shared_ptr<Editor::NodeHost> FactoryRegister::make(const Json::Value & json, Graph::Editor::Patch * parentPatch) {
			const auto nodeTypeName = json["NodeTypeName"].asString();

			auto factory = FactoryRegister::X().get(nodeTypeName);
			if (!factory) {
				throw(Exception("FactoryRegister::make : Missing Factory for Node type " + nodeTypeName));
			}

			auto node = factory->makeUntyped();
			node->setParentPatch(parentPatch);
			node->init();

			node->setName(json["Name"].asString());
			try {
				node->deserialize(json["Content"]);
			}
			RULR_CATCH_ALL_TO_ALERT // don't fail on bad deserialize, just notify user what went wrong

			auto nodeHost = make_shared<Editor::NodeHost>(node);

			ofRectangle bounds;
			json["Bounds"] >> bounds;
			nodeHost->setBounds(bounds);
			
			return nodeHost;
		}
	}
}
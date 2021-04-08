#include "pch_RulrCore.h"
#include "FactoryRegister.h"

#include "ofxRulr/Exception.h"

OFXSINGLETON_DEFINE(ofxRulr::Graph::FactoryRegister);

namespace ofxRulr {
	namespace Graph {
#pragma mark FactoryRegister
		//----------
		shared_ptr<Editor::NodeHost> FactoryRegister::make(const nlohmann::json & json) {
			std::string nodeTypeName;
			json["NodeTypeName"].get_to(nodeTypeName);

			auto factory = FactoryRegister::X().get(nodeTypeName);
			if (!factory) {
				throw(Exception("FactoryRegister::make : Missing Factory for Node type " + nodeTypeName));
			}

			auto node = factory->makeUntyped();
			node->init();

			{
				std::string name;
				json["Name"].get_to(name);
				node->setName(name);
			}
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
#include "Factory.h"
#include "ofxRulr/Exception.h"
#include "ofSystemUtils.h"

namespace ofxRulr {
	namespace Graph {
#pragma mark FactoryRegister
		//----------
		OFXPLUGIN_FACTORY_REGISTER_SINGLETON_SOURCE(FactoryRegister);

		//----------
		shared_ptr<Editor::NodeHost> FactoryRegister::make(const Json::Value & json) {
			const auto nodeTypeName = json["NodeTypeName"].asString();

			auto factory = FactoryRegister::X().get(nodeTypeName);
			if (!factory) {
				throw(Exception("FactoryRegister::make : Missing Factory for Node type " + nodeTypeName));
			}

			auto node = factory->makeUntyped();

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

		//----------
		ofImage & FactoryRegister::getIcon(const shared_ptr<BaseFactory> & factory) {
			auto findIcon = this->icons.find(factory.get());
			if (findIcon != this->icons.end()) {
				//we have the icon
				return *findIcon->second;
			}
			else {
				//we don't have the icon
				if (factory) {
					//make the icon
					auto & icon = factory->makeUntyped()->getIcon();
					this->icons.insert(pair<BaseFactory *, ofImage *>(factory.get(), & icon));
					return icon;
				} else {
					throw(ofxRulr::Exception("Failed to get icon. No factory available."));
				}
			}
		}
	}
}
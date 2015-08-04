#pragma once

#include "ofxPlugin.h"

#include "ofxRulr/Nodes/Base.h"
#include "Editor/NodeHost.h"

#include "../Utils/Initialiser.h"

#define RULR_DECLARE_NODE(NodeType) ofxRulr::Graph::FactoryRegister::X().add<NodeType>();

namespace ofxRulr {
	namespace Graph {
		//----------
		class FactoryRegister : public ofxPlugin::FactoryRegister<Nodes::Base> {
			OFXPLUGIN_FACTORY_REGISTER_SINGLETON_HEADER(FactoryRegister)
		public:
			///Make a NodeHost and Node based on a saved/pasted Json value
			shared_ptr<Editor::NodeHost> make(const Json::Value &);
		};

		//----------
		typedef FactoryRegister::BaseFactory BaseFactory;
	}
}
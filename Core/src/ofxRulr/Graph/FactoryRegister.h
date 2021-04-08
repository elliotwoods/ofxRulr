#pragma once

#include "ofxPlugin.h"

#include "ofxRulr/Nodes/Base.h"
#include "Editor/NodeHost.h"

#include "../Utils/Initialiser.h"
#include "../Utils/Constants.h"

#define RULR_DECLARE_NODE(NodeType) ofxRulr::Graph::FactoryRegister::X().add<NodeType>();

namespace ofxRulr {
	namespace Graph {
		//----------
		class RULR_EXPORTS FactoryRegister : public ofxPlugin::FactoryRegister<Nodes::Base>, public ofxSingleton::Singleton<FactoryRegister> {
		public:
			///Make a NodeHost and Node based on a saved/pasted Json value
			shared_ptr<Editor::NodeHost> make(const nlohmann::json &);
		};

		//----------
		typedef FactoryRegister::BaseFactory BaseFactory;
	}
}
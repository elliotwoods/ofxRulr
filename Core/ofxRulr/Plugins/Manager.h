#pragma once

#include <functional>
#include <vector>

#include "../Graph/Factory.h"
using namespace std;

#define RULR_PLUGIN_DECLARE(X) Plugins::Manager::Declaration<X> X ## _Declarer;

namespace ofxRulr {
	namespace Plugins {
		class Manager {
			static Manager * singleton;
		public:
			static Manager & X();

			template<typename NodeType>
			class Declaration {
			public:
				Declaration() {
					Manager::X().declareNode<NodeType>();
				}
			};

			Manager();
			
			template<typename NodeType>
			void declareNode() {
				ofxRulr::Graph::FactoryRegister::X().add<NodeType>();
			};

			void registerFactories();
		protected:
			vector<std::function<void()>> factoryInitialisers;
		};

		//short hand for the above
		template<typename NodeType>
		void declareNode() {
			Manager::X().declareNode<NodeType>();
		}
	}
}
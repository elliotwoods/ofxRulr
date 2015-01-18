#pragma once

#include <functional>
#include <vector>

#include "../Graph/Factory.h"
using namespace std;

#define OFXDIGITALEMULSION_EXTERNAL_DECLARE(X) External::Manager::Declaration<X> X ## _Declarer;

namespace ofxDigitalEmulsion {
	namespace External {
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
				ofxDigitalEmulsion::Graph::FactoryRegister::X().add<NodeType>();
			};

			void registerFactories();
		protected:
			vector<std::function<void()>> factoryInitialisers;
		};
	}
}
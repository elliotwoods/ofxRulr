#pragma once

#include "../Utils/Set.h"

#include <string>
#include <memory>
#include <vector>

using namespace std;

namespace ofxDigitalEmulsion {
	namespace Graph {
		class BasePin {
		public:
			BasePin(string name);
			virtual string getType() = 0;
			virtual string getName() const;
		private:
			const string name;
		};

		template<typename NodeType>
		class Pin : public BasePin {
		public:
			Pin(string name) : BasePin(name) { }
			Pin() : BasePin(this->getType()) { }

			string getType() override { return NodeType().getTypeName(); }
			void connect(shared_ptr<NodeType> node) {
				this->connection = node;
			}
			shared_ptr<NodeType> getConnection();
		protected:
			shared_ptr<NodeType> connection;
		};

		typedef Utils::Set<BasePin> PinSet;
	}
}

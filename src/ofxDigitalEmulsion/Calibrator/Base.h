#pragma once

#include "../Item/Base.h"
#include <string>
#include <vector>
#include <memory>

using namespace std;

namespace ofxDigitalEmulsion {
	namespace Calibrator {
		class BasePin {
		public:
			virtual string getType() = 0;
		protected:
		};

		template<typename ItemType>
		class Pin {
		public:
			string getType() override { return ItemType().getTypeName(); }
			void connect(shared_ptr<ItemType>);
			shared_ptr<ItemType> getConnection();
		protected:
			shared_ptr<ItemType> connection;
		};

		class PinSet : public vector<shared_ptr<BasePin> > {
		public:
			template<typename ItemType>
			getPinOfType() {
				cast to type
				
			}
		}

		class Base {
		public:
			virtual string getTypeName() = 0;

			virtual PinSet getPins() = 0;
		}
	}
}
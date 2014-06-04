#pragma once

#include "Camera.h"
#include "Projector.h"
#include "../Utils/Set.h"

namespace ofxDigitalEmulsion {
	namespace Item {
		class World : public Utils::Set<Item::Base> {
		public:
			template<typename DeviceType>
			shared_ptr<DeviceType> makeAndAdd() {
				auto device = shared_ptr<DeviceType>(new DeviceType());
				this->push_back(device);

				return device;
				return shared_ptr<DeviceType>();
			}
		};
	}
}
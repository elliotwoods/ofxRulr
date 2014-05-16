#pragma once

#include "Camera.h"
#include "Projector.h"
#include "../Utils/Set.h"

namespace ofxDigitalEmulsion {
	namespace Device {
		class World : public Utils::Set<Device::Base> {
		public:
			template<typename DeviceType>
			shared_ptr<DeviceType> add() {
				auto device = shared_ptr<DeviceType>(new DeviceType());
				this->push_back(device);
				return device;
				return shared_ptr<DeviceType>();
			}

			shared_ptr<Camera> addCamera();
			shared_ptr<Projector> addProjector();
		};
	}
}
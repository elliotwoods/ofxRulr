#include "Factory.h"
#include "../Device/ProjectorOutput.h"

#include "ofxDigitalEmulsion/Item/Camera.h"
#include "ofxDigitalEmulsion/Item/Projector.h"
#include "ofxDigitalEmulsion/Item/Board.h"
#include "ofxDigitalEmulsion/Device/ProjectorOutput.h"
#include "ofxDigitalEmulsion/Procedure/Calibrate/CameraIntrinsics.h"
#include "ofxDigitalEmulsion/Procedure/Calibrate/ProjectorIntrinsicsExtrinsics.h"
#include "ofxDigitalEmulsion/Procedure/Calibrate/HomographyFromGraycode.h"
#include "ofxDigitalEmulsion/Procedure/Scan/Graycode.h"
#include "ofxDigitalEmulsion/Procedure/Triangulate.h"

namespace ofxDigitalEmulsion {
	namespace Graph {
		//----------
		FactoryRegister * FactoryRegister::singleton = 0;

		//----------
		FactoryRegister & FactoryRegister::X() {
			if (!FactoryRegister::singleton) {
				auto factoryRegister = new FactoryRegister();
				FactoryRegister::singleton = factoryRegister;

				factoryRegister->add(make_shared<Factory<Item::Camera>>());
				factoryRegister->add(make_shared<Factory<Item::Projector>>());
				factoryRegister->add(make_shared<Factory<Item::Board>>());
				factoryRegister->add(make_shared<Factory<Device::ProjectorOutput>>());
				factoryRegister->add(make_shared<Factory<Procedure::Calibrate::CameraIntrinsics>>());
				factoryRegister->add(make_shared<Factory<Procedure::Calibrate::ProjectorIntrinsicsExtrinsics>>());
				factoryRegister->add(make_shared<Factory<Procedure::Calibrate::HomographyFromGraycode>>());
				factoryRegister->add(make_shared<Factory<Procedure::Scan::Graycode>>());
				factoryRegister->add(make_shared<Factory<Procedure::Triangulate>>());
			}

			return *singleton;
		}

		//----------
		void FactoryRegister::add(shared_ptr<BaseFactory> factory) {
			this->insert(pair<string, shared_ptr<BaseFactory>>(factory->getNodeTypeName(), factory));
		}

		//----------
		shared_ptr<BaseFactory> FactoryRegister::get(string nodeTypeName) {
			auto findFactory = this->find(nodeTypeName);
			if (findFactory == this->end()) {
				return shared_ptr<BaseFactory>();
			}
			else {
				return findFactory->second;
			}
		}
	}
}
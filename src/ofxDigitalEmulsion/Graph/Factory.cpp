#include "Factory.h"

#include "ofxDigitalEmulsion/Item/Board.h"
#include "ofxDigitalEmulsion/Item/Camera.h"
#include "ofxDigitalEmulsion/Item/Projector.h"
//#include "ofxDigitalEmulsion/Item/Model.h"
#include "ofxDigitalEmulsion/Device/VideoOutput.h"
#include "ofxDigitalEmulsion/Procedure/Calibrate/CameraIntrinsics.h"
#include "ofxDigitalEmulsion/Procedure/Calibrate/ProjectorIntrinsicsExtrinsics.h"
#include "ofxDigitalEmulsion/Procedure/Calibrate/HomographyFromGraycode.h"
#include "ofxDigitalEmulsion/Procedure/Scan/Graycode.h"
#include "ofxDigitalEmulsion/Procedure/Triangulate.h"

namespace ofxDigitalEmulsion {
	namespace Graph {
#pragma mark BaseFactory
		//----------
		BaseFactory::BaseFactory() {
			this->icon = nullptr;
		}

		//----------
		ofImage & BaseFactory::getIcon() {
			if (!this->icon) {
				auto node = this->makeUninitialised();
				this->icon = & node->getIcon();
			}
			return *this->icon;
		}

#pragma mark FactoryRegister
		//----------
		FactoryRegister * FactoryRegister::singleton = 0;

		//----------
		FactoryRegister & FactoryRegister::X() {
			if (!FactoryRegister::singleton) {
				auto factoryRegister = new FactoryRegister();
				FactoryRegister::singleton = factoryRegister;
			}

			return *singleton;
		}

		//----------
		FactoryRegister::FactoryRegister() {
			this->add<Item::Board>();
			this->add<Item::Camera>();
			this->add<Item::Projector>();
			//this->add<Item::Model>();
			this->add<Device::VideoOutput>();
			this->add<Procedure::Calibrate::CameraIntrinsics>();
			//factoryRegister->add<Procedure::Calibrate::ProjectorIntrinsicsExtrinsics>();
			this->add<Procedure::Calibrate::HomographyFromGraycode>();
			this->add<Procedure::Scan::Graycode>();
			this->add<Procedure::Triangulate>();
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

		//----------
		shared_ptr<Editor::NodeHost> FactoryRegister::make(const Json::Value & json) {
			const auto nodeTypeName = json["NodeTypeName"].asString();

			auto factory = FactoryRegister::X().get(nodeTypeName);
			if (!factory) {
				throw(Utils::Exception("FactoryRegister::make : Missing Factory for Node type " + nodeTypeName));
			}

			auto node = factory->make();
			node->deserialize(json["Content"]);
			node->setName(json["Name"].asString());

			auto nodeHost = make_shared<Editor::NodeHost>(node);
			
			ofRectangle bounds;
			json["Bounds"] >> bounds;
			nodeHost->setBounds(bounds);
			
			return nodeHost;
		}
	}
}
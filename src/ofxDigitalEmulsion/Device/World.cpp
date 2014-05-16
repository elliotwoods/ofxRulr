#include "World.h"

namespace ofxDigitalEmulsion {
	namespace Device {
		//----------
		shared_ptr<Camera> World::addCamera() {
			return this->add<Camera>();
		}

		//----------
		shared_ptr<Projector> World::addProjector() {
			return this->add<Projector>();
		}
	}
}
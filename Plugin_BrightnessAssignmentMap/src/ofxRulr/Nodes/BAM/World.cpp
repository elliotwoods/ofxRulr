#include "pch_Plugin_BrightnessAssignmentMap.h"

namespace ofxRulr {
	namespace Nodes {
		namespace BAM {
			//----------
			World::World() {
				RULR_NODE_INIT_LISTENER;
			}

			//----------
			std::string World::getTypeName() const {
				return "BAM::World";
			}

			//----------
			void World::init() {
				this->addInput<Nodes::Base>("Scene");
			}

			//----------
			void World::registerProjector(shared_ptr<Projector> projector) {
				this->projectors.emplace_back(projector);
			}

			//----------
			void World::unregisterProjector(shared_ptr<Projector> projector) {
				for (auto it = this->projectors.begin(); it != this->projectors.end();) {
					auto otherProjector = it->lock();
					if (!otherProjector || otherProjector == projector) {
						it = this->projectors.erase(it);
					}
					else {
						++it;
					}
				}
			}

			//----------
			vector<shared_ptr<Projector>> World::getProjectors() const {
				//return only active projectors

				vector<shared_ptr<Projector>> projectors;
				for (auto projectorWeak : this->projectors) {
					auto projector = projectorWeak.lock();
					if (projector) {
						projectors.emplace_back(projector);
					}
				}
				return projectors;
			}
		}
	}
}
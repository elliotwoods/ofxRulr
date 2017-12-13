#include "pch_Plugin_LSS.h"

namespace ofxRulr {
	namespace Nodes {
		namespace LSS {
			//----------
			World::World() {
				RULR_NODE_INIT_LISTENER;
			}

			//----------
			string World::getTypeName() const {
				return "LSS::World";
			}

			//----------
			void World::init() {

			}

			//----------
			void World::add(shared_ptr<Projector> projector) {
				this->projectors.push_back(projector);
			}

			//----------
			void World::remove(shared_ptr<Projector> projector) {
				for (auto it = this->projectors.begin(); it != this->projectors.end();) {
					auto otherProjector = it->lock();
					if (otherProjector) {
						if (otherProjector != projector) {
							it++;
							continue;;
						}
					}

					it = this->projectors.erase(it);
				}
			}

			//----------
			vector<shared_ptr<Projector>> World::getProjectors() const {
				multimap<int, shared_ptr<Projector>> resultMap;
				for (auto projectorWeak : this->projectors) {
					auto projector = projectorWeak.lock();
					if (projector) {
						resultMap.emplace(projector->getProjectorIndex(), projector);
					}
				}

				vector<shared_ptr<Projector>> resultVector;
				for (const auto & it : resultMap) {
					resultVector.push_back(it.second);
				}

				return resultVector;
			}
		}
	}
}
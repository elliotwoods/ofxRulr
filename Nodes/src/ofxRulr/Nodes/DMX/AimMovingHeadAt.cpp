#include "AimMovingHeadAt.h"

#include "ofxRulr/Nodes/Item/RigidBody.h"
#include "ofxRulr/Nodes/DMX/MovingHead.h"

namespace ofxRulr {
	namespace Nodes {
		namespace DMX {
			//----------
			AimMovingHeadAt::AimMovingHeadAt() {
				RULR_NODE_INIT_LISTENER;
			}

			//----------
			void AimMovingHeadAt::init() {
				RULR_NODE_UPDATE_LISTENER;

				this->addInput<MovingHead>();
				this->addInput<Item::RigidBody>("Target");
			}

			//----------
			string AimMovingHeadAt::getTypeName() const {
				return "DMX::AimMovingHeadAtTarget";
			}

			//----------
			void AimMovingHeadAt::update() {
				auto movingHead = this->getInput<MovingHead>();
				auto target = this->getInput<Item::RigidBody>("Target");
				if (target && movingHead) {
					try {
						movingHead->lookAt(target->getPosition());
					}
					RULR_CATCH_ALL_TO_ERROR;
				}
			}
		}
	}
}
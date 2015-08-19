#include "AimMovingHeadAt.h"

#include "ofxRulr/Nodes/Item/RigidBody.h"
#include "ofxRulr/Nodes/DMX/MovingHead.h"

#include "ofxCvGui/Widgets/Toggle.h"

using namespace ofxCvGui;

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
				RULR_NODE_INSPECTOR_LISTENER;
				RULR_NODE_SERIALIZATION_LISTENERS;

				this->addInput<MovingHead>();
				this->addInput<Item::RigidBody>("Target");

				this->ignoreBlankTransform.set("Ignore blank transform", true);
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

			//----------
			void AimMovingHeadAt::serialize(Json::Value & json) {
				Utils::Serializable::serialize(this->ignoreBlankTransform, json);
			}

			//----------
			void AimMovingHeadAt::deserialize(const Json::Value & json) {
				Utils::Serializable::deserialize(this->ignoreBlankTransform, json);
			}

			//----------
			void AimMovingHeadAt::populateInspector(ofxCvGui::ElementGroupPtr inspector) {
				inspector->add(Widgets::Toggle::make(this->ignoreBlankTransform));
			}
		}
	}
}
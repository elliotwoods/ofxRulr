#include "AimMovingHeadAt.h"

#include "ofxRulr/Nodes/Item/RigidBody.h"
#include "ofxRulr/Nodes/DMX/MovingHead.h"

#include "ofxCvGui/Widgets/Toggle.h"
#include "ofxCvGui/Widgets/Slider.h"
#include "ofxCvGui/Widgets/Title.h"

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

				{
					this->prediction.enabled.set("Enabled", false);
					this->prediction.delay.set("Delay", 0.0f, -5.0f, 0.0f);
				}
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
						bool doIt = true;

						//check if transform is blank before using it (blank can be an indicator of bad tracking)
						if (this->ignoreBlankTransform) {
							if (target->getTransform().isIdentity()) {
								doIt = false;
							}
						}

						//perform the move
						if (doIt) {
							movingHead->lookAt(target->getPosition());
						}
					}
					RULR_CATCH_ALL_TO_ERROR;
				}
			}

			//----------
			void AimMovingHeadAt::serialize(Json::Value & json) {
				Utils::Serializable::serialize(this->ignoreBlankTransform, json);

				auto & jsonPrediction = json["prediction"];
				{
					Utils::Serializable::serialize(this->prediction.enabled, json["prediction"]);
					Utils::Serializable::serialize(this->prediction.delay, json["prediction"]);
				}
			}

			//----------
			void AimMovingHeadAt::deserialize(const Json::Value & json) {
				Utils::Serializable::deserialize(this->ignoreBlankTransform, json);
				const auto & jsonPrediction = json["prediction"];
				{
					Utils::Serializable::deserialize(this->prediction.enabled, json["prediction"]);
					Utils::Serializable::deserialize(this->prediction.delay, json["prediction"]);
				}
			}

			//----------
			void AimMovingHeadAt::populateInspector(ofxCvGui::ElementGroupPtr inspector) {
				inspector->add(Widgets::Toggle::make(this->ignoreBlankTransform));

				inspector->add(Widgets::Title::make("Prediction"));
				{
					inspector->add(Widgets::Toggle::make(this->prediction.enabled));
					inspector->add(Widgets::Slider::make(this->prediction.delay));
				}
			}
		}
	}
}
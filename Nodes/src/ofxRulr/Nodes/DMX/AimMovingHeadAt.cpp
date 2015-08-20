#include "AimMovingHeadAt.h"

#include "ofxRulr/Nodes/Item/RigidBody.h"
#include "ofxRulr/Nodes/DMX/MovingHead.h"

#include "ofxCvGui/Widgets/Toggle.h"
#include "ofxCvGui/Widgets/Slider.h"
#include "ofxCvGui/Widgets/Title.h"

#include "ofxRulr/Utils/PolyFit.h"

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
					this->prediction.historySize.set("History size", 5, 0, 100);
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
						ofVec3f aimPosition = target->getPosition();

						//check if transform is blank before using it (blank can be an indicator of bad tracking)
						if (this->ignoreBlankTransform) {
							if (target->getTransform().isIdentity()) {
								doIt = false;
							}
						}

						//steve jobs mode
						if (this->prediction.enabled) {
							auto now = ofGetElapsedTimef();
							this->prediction.history[now] = aimPosition;
							while (this->prediction.history.size() > this->prediction.historySize && !this->prediction.history.empty()) {
								this->prediction.history.erase(this->prediction.history.begin());
							}

//#pragma omp parallel for
							auto delay = this->prediction.delay.get();
							for (int i = 0; i < 3; i++) {
								vector<ofVec2f> x_vs_t;
								for (auto historyFrame : this->prediction.history) {
									x_vs_t.push_back(ofVec2f(historyFrame.first, historyFrame.second[i]));
								}
								auto model = Utils::PolyFit::fit(x_vs_t, 1);
								auto predictedPosition = Utils::PolyFit::evaluate(model, now - delay);
								//watch out for NaN's
								if (predictedPosition == predictedPosition) {
									aimPosition[i] = predictedPosition;
								}
							}
						}
						//perform the move
						if (doIt) {
							movingHead->lookAt(aimPosition);
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
					Utils::Serializable::serialize(this->prediction.historySize, json["prediction"]);
				}
			}

			//----------
			void AimMovingHeadAt::deserialize(const Json::Value & json) {
				Utils::Serializable::deserialize(this->ignoreBlankTransform, json);
				const auto & jsonPrediction = json["prediction"];
				{
					Utils::Serializable::deserialize(this->prediction.enabled, json["prediction"]);
					Utils::Serializable::deserialize(this->prediction.delay, json["prediction"]);
					Utils::Serializable::deserialize(this->prediction.historySize, json["prediction"]);
				}
			}

			//----------
			void AimMovingHeadAt::populateInspector(ofxCvGui::ElementGroupPtr inspector) {
				inspector->add(Widgets::Toggle::make(this->ignoreBlankTransform));

				inspector->add(Widgets::Title::make("Steve Jobs Mode", Widgets::Title::Level::H2));
				{
					inspector->add(Widgets::Title::make("Prediction", Widgets::Title::Level::H3));
					inspector->add(Widgets::Toggle::make(this->prediction.enabled));
					inspector->add(Widgets::Slider::make(this->prediction.delay));

					auto historySizeWidget = Widgets::Slider::make(this->prediction.historySize);
					historySizeWidget->addIntValidator();
					inspector->add(historySizeWidget);
				}
			}
		}
	}
}
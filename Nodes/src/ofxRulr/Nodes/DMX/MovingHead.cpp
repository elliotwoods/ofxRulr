#include "MovingHead.h"

#include "ofxCvGui/Widgets/Slider.h"
#include "ofxCvGui/Widgets/EditableValue.h"
#include "ofxCvGui/Widgets/Title.h"
#include "ofxCvGui/Widgets/Toggle.h"
#include "ofxCvGui/Widgets/Button.h"

using namespace ofxCvGui;

namespace ofxRulr {
	namespace Nodes {
		namespace DMX {
			//----------
			MovingHead::MovingHead() {
				RULR_NODE_INIT_LISTENER;
			}

			//----------
			void MovingHead::init() {
				RULR_NODE_SERIALIZATION_LISTENERS;
				RULR_NODE_INSPECTOR_LISTENER;

				this->pan.set("Pan", 0.0f, -180.0f, 180.0f);
				this->tilt.set("Tilt", 0.0f, -100.0f, 100.0f);
				this->brightness.set("Brightness", 0.0f, 0.0f, 1.0f);
				this->power.set("Power", false);
				this->powerCircuit.set("Power circuit (mutex)", "main ring");
				this->pauseBetweenPowerUps.set("Pause between power ups [s]", 5);
			}

			//----------
			string MovingHead::getTypeName() const {
				return "DMX::MovingHead";
			}

			//----------
			void MovingHead::serialize(Json::Value & json) {
				Utils::Serializable::serialize(this->pan, json);
				Utils::Serializable::serialize(this->tilt, json);
				Utils::Serializable::serialize(this->brightness, json);
				Utils::Serializable::serialize(this->power, json);
				Utils::Serializable::serialize(this->powerCircuit, json);
				Utils::Serializable::serialize(this->pauseBetweenPowerUps, json);
			}

			//----------
			void MovingHead::deserialize(const Json::Value & json) {
				Utils::Serializable::deserialize(this->pan, json);
				Utils::Serializable::deserialize(this->tilt, json);
				Utils::Serializable::deserialize(this->brightness, json);
				Utils::Serializable::deserialize(this->power, json);
				Utils::Serializable::deserialize(this->powerCircuit, json);
				Utils::Serializable::deserialize(this->pauseBetweenPowerUps, json);
			}

			//----------
			ofVec2f MovingHead::getPanTilt() const {
				return ofVec2f(this->pan, this->tilt);
			}

			//----------
			ofVec2f MovingHead::getPanTiltForTarget(const ofVec3f & worldSpacePoint, bool closestToCurrent) const {
				auto worldToObject = this->getTransform().getInverse();
				auto objectSpacePoint = worldSpacePoint * worldToObject;
				auto pan = atan2(objectSpacePoint.z, objectSpacePoint.x) * RAD_TO_DEG - 90.0f;
				auto tilt = acos(-objectSpacePoint.y / objectSpacePoint.length()) * RAD_TO_DEG; // will always produce positive tilt

				if (closestToCurrent) {
					//we want to find the fastest route to it, for every 180 degrees of pan there is a valid solution, let's choose the one with the smallest pan always
					auto halfRotationsMin = ceil(this->pan.getMin() / 180.0f);
					auto halfRotationsMax = ceil(this->pan.getMax() / 180.0f) + 1;
					map<float, ofVec2f> solutions; // panDistance, (pan,tilt)
					for (int halfRotation = halfRotationsMin; halfRotation < halfRotationsMax; halfRotation++) {
						//for each hemisphere, find the solution, check if it's in valid range
						float searchPan = (halfRotation * 180) + pan;
						float searchTilt;
						if (halfRotation % 2 == 0) {
							//even, keep tilt
							searchTilt = tilt;
						}
						else {
							//odd, flip tilt
							searchTilt = -tilt;
						}
						if (searchPan >= this->pan.getMin() && searchPan <= this->pan.getMax() && searchTilt >= this->tilt.getMin() && searchTilt <= this->tilt.getMax()) {
							//it's a valid solution
							auto panDistance = abs(searchPan - this->pan);
							pair<float, ofVec2f> solution(panDistance, ofVec2f(pan, tilt));
							solutions.insert(solution);
						}
					}

					if (solutions.empty()) {
						throw(ofxRulr::Exception("No valid solutions found to aim MovingHead '" + this->getName() + "' at target (" + ofToString(worldSpacePoint) + ")"));
					}

					auto solution = solutions.begin();
					return solution->second;
				}
			}

			//----------
			void MovingHead::lookAt(const ofVec3f & worldSpacePoint) {
				this->setPanTilt(this->getPanTiltForTarget(worldSpacePoint, true));
			}
		
			//----------
			void MovingHead::setPanTilt(const ofVec2f & panTilt) {
				this->pan = panTilt.x;
				this->tilt = panTilt.y;
			}

			//----------
			void MovingHead::populateInspector(ofxCvGui::ElementGroupPtr inspector) {
				inspector->add(Widgets::Title::make("DMX::MovingHead", Widgets::Title::Level::H2));
				inspector->add(Widgets::Slider::make(this->pan));
				inspector->add(Widgets::Slider::make(this->tilt));
				inspector->add(Widgets::Slider::make(this->brightness));
				inspector->add(Widgets::Toggle::make(this->power));
				inspector->add(Widgets::EditableValue<string>::make(this->powerCircuit));
				inspector->add(Widgets::EditableValue<int>::make(this->pauseBetweenPowerUps));
				inspector->add(Widgets::Button::make("Home", [this]() {
					this->pan.set(0.0f);
					this->tilt.set(0.0f);
				}));
			}
		}
	}
}
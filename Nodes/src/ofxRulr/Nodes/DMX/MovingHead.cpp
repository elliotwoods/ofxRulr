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
				RULR_NODE_UPDATE_LISTENER;
				RULR_NODE_SERIALIZATION_LISTENERS;
				RULR_NODE_INSPECTOR_LISTENER;

				this->pan.set("Pan", 0.0f, -180.0f, 180.0f);
				this->tilt.set("Tilt", 0.0f, -100.0f, 100.0f);
				this->brightness.set("Brightness", 0.0f, 0.0f, 1.0f);
				this->power.set("Power", false);
				this->powerCircuit.set("Power circuit (mutex)", "main ring");
				this->pauseBetweenPowerUps.set("Pause between power ups [s]", 5);

				this->powerStateSignal = false;
			}

			//----------
			string MovingHead::getTypeName() const {
				return "DMX::MovingHead";
			}
			
			//----------
			void MovingHead::update() {
				if (this->powerStateSignal != this->power) {
					if (this->power) {
						//global store of power circuit control
						static map<string, float> lastPowerUpPerCircuit;
						static ofMutex lastPowerUpPerCircuitMutex;

						//lock to make sure nobody else is doing the same thing right now
						lastPowerUpPerCircuitMutex.lock();
						{
							//try and find this power circuit
							auto findCircuit = lastPowerUpPerCircuit.find(this->powerCircuit.get());
							if (findCircuit == lastPowerUpPerCircuit.end()) {
								//register this circuit, say the last spark on it was really long ago
								lastPowerUpPerCircuit.insert(pair<string, float>(this->powerCircuit.get(), std::numeric_limits<float>::min()));
							}
							
							//check if we've had a power on event too recently
							auto now = ofGetElapsedTimef();
							auto & lastPowerUpOnThisCircuit = lastPowerUpPerCircuit[this->powerCircuit.get()];
							if (now - this->pauseBetweenPowerUps.get() > lastPowerUpOnThisCircuit) {
								//ok it's time, let's boot
								this->powerStateSignal = true;
								lastPowerUpOnThisCircuit = now;
							}
							else {
								//no let's wait and have another see next frame
							}
						}
						lastPowerUpPerCircuitMutex.unlock();
					}
					else {
						this->powerStateSignal = false;
					}

				}
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
			float MovingHead::getMaxTilt() const {
				return this->tilt.getMax();
			}

			//----------
			ofVec2f MovingHead::getPanTiltForTarget(const ofVec3f & worldSpacePoint, bool navigationEnabled) const {
				auto worldToObject = this->getTransform().getInverse();
				auto objectSpacePoint = worldSpacePoint * worldToObject;
				
				auto panTilt = MovingHead::getPanTiltForTargetInObjectSpace(objectSpacePoint);

				if (navigationEnabled) {
					//we want to find the fastest route to it, for every 180 degrees of pan there is a valid solution, let's choose the one with the smallest pan always
					auto halfRotationsMin = ceil(this->pan.getMin() / 180.0f);
					auto halfRotationsMax = ceil(this->pan.getMax() / 180.0f) + 1;
					map<float, ofVec2f> solutions; // panDistance, (pan,tilt)
					for (int halfRotation = halfRotationsMin; halfRotation < halfRotationsMax; halfRotation++) {
						//for each hemisphere, find the solution, check if it's in valid range
						float searchPan = (halfRotation * 180) + panTilt.x;
						float searchTilt;
						if (halfRotation % 2 == 0) {
							//even, keep tilt
							searchTilt = panTilt.y;
						}
						else {
							//odd, flip tilt
							searchTilt = -panTilt.y;
						}
						if (searchPan >= this->pan.getMin() && searchPan <= this->pan.getMax() && searchTilt >= this->tilt.getMin() && searchTilt <= this->tilt.getMax()) {
							//it's a valid solution
							auto panDistance = abs(searchPan - this->pan);
							pair<float, ofVec2f> solution(panDistance, ofVec2f(searchPan, searchTilt));
							solutions.insert(solution);
						}
					}

					if (solutions.empty()) {
						throw(ofxRulr::Exception("No valid solutions found to aim MovingHead '" + this->getName() + "' at target (" + ofToString(worldSpacePoint) + ")"));
					}

					auto solution = solutions.begin();
					return solution->second;
				}
				else {
					return panTilt;
				}
			}

			//----------
			ofVec2f MovingHead::getPanTiltForTargetInObjectSpace(const ofVec3f & objectSpacePoint) {
				auto pan = atan2(objectSpacePoint.z, objectSpacePoint.x) * RAD_TO_DEG - 90.0f;
				auto tilt = acos(-objectSpacePoint.y / objectSpacePoint.length()) * RAD_TO_DEG; // will always produce positive tilt
				return ofVec2f(pan, tilt);
			}

			//----------
			void MovingHead::lookAt(const ofVec3f & worldSpacePoint) {
				const auto panTilt = this->getPanTiltForTarget(worldSpacePoint, true);
				this->setPanTilt(panTilt);
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
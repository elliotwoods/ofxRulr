#include "pch_RulrNodes.h"
#include "MovingHead.h"

#include "ofxCvGui/Widgets/Slider.h"
#include "ofxCvGui/Widgets/EditableValue.h"
#include "ofxCvGui/Widgets/Title.h"
#include "ofxCvGui/Widgets/Toggle.h"
#include "ofxCvGui/Widgets/Indicator.h"
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

				this->powerStateSignal = false;
			}

			//----------
			string MovingHead::getTypeName() const {
				return "DMX::MovingHead";
			}
			
			//----------
			void MovingHead::update() {
				if (this->powerStateSignal != this->parameters.power) {
					if (this->parameters.power) {
						//global store of power circuit control
						static map<string, float> lastPowerUpPerCircuit;
						static ofMutex lastPowerUpPerCircuitMutex;

						//lock to make sure nobody else is doing the same thing right now
						lastPowerUpPerCircuitMutex.lock();
						{
							//try and find this power circuit
							auto findCircuit = lastPowerUpPerCircuit.find(this->parameters.powerCircuit.get());
							if (findCircuit == lastPowerUpPerCircuit.end()) {
								//register this circuit, say the last spark on it was really long ago
								lastPowerUpPerCircuit.insert(pair<string, float>(this->parameters.powerCircuit.get(), std::numeric_limits<float>::min()));
							}
							
							//check if we've had a power on event too recently
							auto now = ofGetElapsedTimef();
							auto & lastPowerUpOnThisCircuit = lastPowerUpPerCircuit[this->parameters.powerCircuit.get()];
							if (now - this->parameters.pauseBetweenPowerUps.get() > lastPowerUpOnThisCircuit) {
								//ok it's time, let's boot
								this->powerStateSignal = true;
								lastPowerUpOnThisCircuit = now;
								cout << "Powering up " << this->getName() << " at " << now << "s" << endl;
							}
							else {
								//no let's wait and have another see next frame
								float x = 0.0f; //in case we want to put a break point here
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
			void MovingHead::serialize(nlohmann::json & json) {
				Utils::serialize(json, this->parameters);
			}

			//----------
			void MovingHead::deserialize(const nlohmann::json & json) {
				Utils::deserialize(json, this->parameters);
			}

			//----------
			glm::vec2 MovingHead::getPanTilt() const {
				return glm::vec2(this->parameters.pan, this->parameters.tilt);
			}

			//----------
			float MovingHead::getMaxTilt() const {
				return this->parameters.tilt.getMax();
			}

			//----------
			glm::vec2 MovingHead::getPanTiltForTarget(const glm::vec3 & worldSpacePoint, bool navigationEnabled) const {
				auto worldToObject = glm::inverse(this->getTransform());
				auto objectSpacePoint = Utils::applyTransform(worldToObject, worldSpacePoint);
				
				auto panTilt = MovingHead::getPanTiltForTargetInObjectSpace(objectSpacePoint, this->parameters.tiltOffset);

				if (navigationEnabled) {
					//we want to find the fastest route to it, for every 180 degrees of pan there is a valid solution, let's choose the one with the smallest pan always
					auto halfRotationsMin = ceil(this->parameters.pan.getMin() / 180.0f);
					auto halfRotationsMax = ceil(this->parameters.pan.getMax() / 180.0f) + 1;
					map<float, glm::vec2> solutions; // panDistance, (pan,tilt)
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
						if (searchPan >= this->parameters.pan.getMin() && searchPan <= this->parameters.pan.getMax() && searchTilt >= this->parameters.tilt.getMin() && searchTilt <= this->parameters.tilt.getMax()) {
							//it's a valid solution
							auto panDistance = abs(searchPan - this->parameters.pan);
							pair<float, glm::vec2> solution(panDistance, glm::vec2(searchPan, searchTilt));
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
			glm::vec2 MovingHead::getPanTiltForTargetInObjectSpace(const glm::vec3 & objectSpacePoint, float tiltOffset) {
				auto pan = atan2(objectSpacePoint.z, objectSpacePoint.x) * RAD_TO_DEG - 90.0f;
				if (pan < -180.0f) {
					pan += 360.0f;
				}

				auto tilt = acos(-objectSpacePoint.y / glm::length(objectSpacePoint)) * RAD_TO_DEG; // will always produce positive tilt
				return glm::vec2(pan, tilt + tiltOffset);
			}

			//----------
			void MovingHead::lookAt(const glm::vec3& worldSpacePoint) {
				const auto panTilt = this->getPanTiltForTarget(worldSpacePoint, false);
				this->setPanTilt(panTilt);
			}
		
			//----------
			void MovingHead::setPanTilt(const glm::vec2 & panTilt) {
				this->parameters.pan = panTilt.x;
				this->parameters.tilt = panTilt.y;
			}

			//----------
			void MovingHead::setBrightness(float brightness) {
				this->parameters.brightness = brightness;
			}

			//----------
			void MovingHead::setIris(float iris) {
				this->parameters.iris = iris;
			}

			//----------
			void MovingHead::setPower(bool power) {
				this->parameters.power = power;
			}

			//----------
			void MovingHead::setHome() {
				this->parameters.pan.set(0.0f);
				this->parameters.tilt.set(0.0f);
			}

			//----------
			void MovingHead::setTiltOffset(float tiltOffset) {
				this->parameters.tiltOffset = tiltOffset;
			}

			//----------
			void MovingHead::copyFrom(shared_ptr<MovingHead> other) {
				this->parameters.pan = other->parameters.pan;
				this->parameters.tilt = other->parameters.tilt;
				this->parameters.brightness = other->parameters.brightness;
				this->parameters.iris = other->parameters.iris;
				this->parameters.power = other->parameters.power;
				this->parameters.powerCircuit = other->parameters.powerCircuit;
				this->parameters.pauseBetweenPowerUps = other->parameters.pauseBetweenPowerUps;
				this->parameters.tiltOffset = other->parameters.tiltOffset;
			}

			//----------
			void MovingHead::populateInspector(ofxCvGui::InspectArguments & inspectArguments) {
				auto inspector = inspectArguments.inspector;
				
				inspector->add(new Widgets::Title("DMX::MovingHead", Widgets::Title::Level::H2));
				inspector->add(new Widgets::Slider(this->parameters.pan));
				inspector->add(new Widgets::Slider(this->parameters.tilt));
				inspector->add(new Widgets::Button("Home", [this]() {
					this->setHome();
				}));
				inspector->add(new Widgets::Slider(this->parameters.brightness));
				inspector->add(new Widgets::Slider(this->parameters.iris));
				inspector->add(new Widgets::Toggle(this->parameters.power));
				inspector->add(new Widgets::EditableValue<string>(this->parameters.powerCircuit));
				inspector->add(new Widgets::EditableValue<int>(this->parameters.pauseBetweenPowerUps));
				inspector->add(new Widgets::Indicator("Power status", [this]() {
					return (Widgets::Indicator::Status) this->powerStateSignal;
				}));
				inspector->add(new Widgets::Slider(this->parameters.tiltOffset));
				
			}
		}
	}
}
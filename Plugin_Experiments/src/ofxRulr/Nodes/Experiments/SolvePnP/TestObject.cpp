#include "pch_Plugin_Experiments.h"

namespace ofxRulr {
	namespace Nodes {
		namespace Experiments {
			namespace SolvePnP {
				//----------
				TestObject::TestObject() {
					RULR_NODE_INIT_LISTENER;
				}

				//----------
				string TestObject::getTypeName() const {
					return "Experiments::SolvePnP::TestObject";
				}

				//----------
				void TestObject::init() {
					RULR_RIGIDBODY_DRAW_OBJECT_LISTENER;
					this->manageParameters(this->parameters);

					this->planeObject.setCenter({ 0.0f, 0.0f, 0.0f });
					this->planeObject.setScale({0.5f, 0.5f});
					this->planeObject.setNormal({ 0.0f, 0.0f, -1.0f });
					this->planeObject.setUp({ 0.0f, +1.0f, 0.0f });
					this->planeObject.setInfinite(false);
				}

				//----------
				void TestObject::drawObject() {
					ofPushMatrix();
					{
						ofScale(this->parameters.diameter);
						this->planeObject.draw();
					}
					ofPopMatrix();
				}


				//----------
				std::vector<glm::vec3> TestObject::getObjectPoints() {
					vector<glm::vec3> points = {
						{ -0.5f, -0.5f, 0.0f }
						,{ +0.5f, -0.5f, 0.0f }
						,{ +0.5f, +0.5f, 0.0f }
						,{ -0.5f, +0.5f, 0.0f }
					};

					//apply diameter
					for (auto & point : points) {
						point = point * this->parameters.diameter.get();
					}

					return points;
				}


				//----------
				std::vector<glm::vec3> TestObject::getWorldPoints() {
					auto points = this->getObjectPoints();

					//apply diameter and transform
					glm::mat4 worldTransform = this->getTransform();
					for (auto & point : points) {
						point = Utils::applyTransform(worldTransform, point);
					}

					return points;
				}
			}
		}
	}
}
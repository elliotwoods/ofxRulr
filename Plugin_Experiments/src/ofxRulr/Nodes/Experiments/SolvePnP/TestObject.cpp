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
				std::vector<ofVec3f> TestObject::getObjectPoints() {
					vector<ofVec3f> points = {
						{ -0.5f, -0.5f, 0.0f }
						,{ +0.5f, -0.5f, 0.0f }
						,{ +0.5f, +0.5f, 0.0f }
						,{ -0.5f, +0.5f, 0.0f }
					};

					//apply diameter
					for (auto & point : points) {
						point = point * this->parameters.diameter;
					}

					return points;
				}


				//----------
				std::vector<ofVec3f> TestObject::getWorldPoints() {
					auto points = this->getObjectPoints();

					//apply diameter and transform
					ofMatrix4x4 worldTransform = this->getTransform();
					for (auto & point : points) {
						point = point * worldTransform;
					}

					return points;
				}
			}
		}
	}
}
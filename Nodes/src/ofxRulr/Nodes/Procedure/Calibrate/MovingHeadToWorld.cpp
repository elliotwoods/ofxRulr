#include "MovingHeadToWorld.h"

#include "ofxRulr/Nodes/DMX/MovingHead.h"
#include "ofxRulr/Nodes/Item/RigidBody.h"

#include "ofxCvGui/Widgets/Button.h"

#include "ofxNonLinearFit.h"

using namespace ofxCvGui;

namespace ofxRulr {
	namespace Nodes {
		namespace Procedure {
			namespace Calibrate {
				//---------
				MovingHeadToWorld::MovingHeadToWorld() {
					RULR_NODE_INIT_LISTENER;
				}

				//---------
				void MovingHeadToWorld::init() {
					this->addInput<Item::RigidBody>("Marker");
					this->addInput<DMX::MovingHead>();
				}

				//---------
				string MovingHeadToWorld::getTypeName() const {
					return "Procedure::Calibrate::MovingHeadToWorld";
				}

				//---------
				void MovingHeadToWorld::serialize(Json::Value & json) {

				}

				//---------
				void MovingHeadToWorld::deserialize(const Json::Value & json) {

				}

				//---------
				void MovingHeadToWorld::populateInspector(ElementGroupPtr inspector) {
					auto addCaptureButton = Widgets::Button::make("Add capture", [this]() {
						try {
							this->addCapture();
						}
						RULR_CATCH_ALL_TO_ALERT

					}, ' ');
				}

				//---------
				void MovingHeadToWorld::drawWorld() {
					ofMesh lines;
					auto movingHead = this->getInput<DMX::MovingHead>();
					auto movingHeadRotation = movingHead ? movingHead->getRotationQuat() : ofQuaternion();

					for (const auto & dataPoint : this->dataPoints) {
						lines.addVertex(dataPoint.world);

						auto vector = ofVec3f(0, 0.1f, 0.0f);
						auto rotation = movingHeadRotation * ofQuaternion(dataPoint.panTilt.x, ofVec3f(0, -1, 0)) * ofQuaternion(dataPoint.panTilt.y, ofVec3f(1,0,0));
						lines.addVertex(vector * rotation + dataPoint.world);
					}
					lines.setMode(ofPrimitiveMode::OF_PRIMITIVE_LINES);
					lines.draw();
				}

				//---------
				void MovingHeadToWorld::addCapture() {
					this->throwIfMissingAnyConnection();
					auto movingHead = this->getInput<DMX::MovingHead>();
					auto target = this->getInput<Item::RigidBody>();

					DataPoint dataPoint = {
						target->getPosition(),
						movingHead->getPanTilt()
					};
				}

				//---------
				class Model : public ofxNonLinearFit::Models::Base<MovingHeadToWorld::DataPoint, Model> {
				public:
					Model() {
						//Fit needs to call this constructor at the start
					}

					Model(shared_ptr<DMX::MovingHead> movingHead) {
						this->movingHead = movingHead;
					}

					typedef MovingHeadToWorld::DataPoint DataPoint;
					unsigned int getParameterCount() const override {
						return 6;
					}

					void resetParameters() override {
						auto parameters = this->getParameters();
						auto position = this->movingHead->getPosition();
						auto rotationEuler = this->movingHead->getRotationEuler();
						for (int i = 0; i < 3; i++) {
							*parameters++ = position[i];
						}
						for (int i = 0; i < 3; i++) {
							*parameters++ = rotationEuler[i];
						}
					}
					
					double getResidual(DataPoint point) const override {
						auto pointEvaluated = point;
						try {
							this->evaluate(pointEvaluated);
						}
						catch (...) {
							//invalid position
							return std::numeric_limits<double>::max();
						}

						return (pointEvaluated.panTilt - point.panTilt).lengthSquared();
					}

					///Warning : can throw exception
					void evaluate(DataPoint & point) const {
						point.panTilt = this->movingHead->getPanTiltForTarget(point.world, false);
					}

					void cacheModel() override {
						if (this->isReady()) {
							auto parameters = this->getParameters();
							auto position = ofVec3f(*parameters++, *parameters++, *parameters++);
							this->movingHead->setPosition(position);
							auto rotationEuler = ofVec3f(*parameters++, *parameters++, *parameters++);
							this->movingHead->setRotationEuler(rotationEuler);
						}
					}
				protected:
					shared_ptr<DMX::MovingHead> movingHead;
				};

				//---------
				void MovingHeadToWorld::calibrate() {
					this->throwIfMissingAConnection<DMX::MovingHead>();

					auto fit = ofxNonLinearFit::Fit<Model>();
					auto model = Model(this->getInput<DMX::MovingHead>());

					double residual;
					fit.optimise(model, &this->dataPoints, &residual);

					this->residual = residual;
				}
			}
		}
	}
}
#include "MovingHeadToWorld.h"

#include "ofxRulr/Nodes/DMX/MovingHead.h"
#include "ofxRulr/Nodes/Item/RigidBody.h"
#include "ofxRulr/Utils/Utils.h"

#include "ofxCvGui/Widgets/LiveValue.h"
#include "ofxCvGui/Widgets/Button.h"

#include "ofxNonLinearFit.h"
#include "../../../addons/ofxGLM/src/ofxGLM.h"

using namespace ofxCvGui;
using namespace ofxGLM;

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
					RULR_NODE_SERIALIZATION_LISTENERS;
					RULR_NODE_INSPECTOR_LISTENER;

					this->addInput<Item::RigidBody>("Marker");
					this->addInput<DMX::MovingHead>();

					auto view = make_shared<Panels::Base>();
					view->onDraw += [this](ofxCvGui::DrawArguments & args) {
						auto movingHead = this->getInput<DMX::MovingHead>();
						if (movingHead) {
							ofPushStyle();
							{
								auto viewWidth = args.localBounds.width;
								auto viewHeight = args.localBounds.height;

								ofSetLineWidth(1.0f);
								//center line
								ofDrawLine(viewWidth / 2.0f, 0.0f, viewWidth / 2.0f, viewHeight);
								//horizon line
								auto horizonHeight = ofMap(90.0f, 0.0f, movingHead->getMaxTilt(), 0.0f, viewHeight);
								ofDrawLine(0.0f, horizonHeight, viewWidth, horizonHeight);

								auto panTilt = movingHead->getPanTilt();
								auto drawX = ofMap(panTilt.x, +180.0f, -180.0f, 0.0f, viewWidth);
								auto drawY = ofMap(panTilt.y, 0.0f, movingHead->getMaxTilt(), 0.0f, viewHeight);

								ofFill();
								ofDrawCircle(drawX, drawY, 5.0f);
							}
							ofPopStyle();
						}
					};
					auto viewWeak = weak_ptr<Panels::Base>(view);
					view->onMouse += [this, viewWeak](MouseArguments & args) {
						auto movingHead = this->getInput<DMX::MovingHead>();
						if (movingHead) {
							auto view = viewWeak.lock();
							if (view) {
								if (args.takeMousePress(view) || args.isDragging(view)) {
									auto pan = ofMap(args.localNormalised.x, 0.0f, 1.0f, +180.0f, -180.0f, true);
									auto tilt = ofMap(args.localNormalised.y, 0.0f, 1.0f, 0.0f, movingHead->getMaxTilt(), true);
									movingHead->setPanTilt(ofVec2f(pan, tilt));
								}
							}
						}
					};
					this->view = view;

					this->residual = 0.0f;
				}

				//---------
				string MovingHeadToWorld::getTypeName() const {
					return "Procedure::Calibrate::MovingHeadToWorld";
				}

				//---------
				void MovingHeadToWorld::serialize(Json::Value & json) {
					auto & jsonDataPoints = json["dataPoints"];
					for (int i = 0; i < this->dataPoints.size(); i++) {
						jsonDataPoints[i]["world"] << this->dataPoints[i].world;
						jsonDataPoints[i]["panTilt"] << this->dataPoints[i].panTilt;
					}
					json["residual"] << this->residual;
				}

				//---------
				void MovingHeadToWorld::deserialize(const Json::Value & json) {
					this->dataPoints.clear();
					const auto & jsonDataPoints = json["dataPoints"];
					for (const auto & jsonDataPoint : jsonDataPoints) {
						DataPoint dataPoint;
						jsonDataPoint["world"] >> dataPoint.world;
						jsonDataPoint["panTilt"] >> dataPoint.panTilt;
						this->dataPoints.push_back(dataPoint);
					}
					json["residual"] >> this->residual;
				}

				//---------
				void MovingHeadToWorld::populateInspector(ElementGroupPtr inspector) {
					auto addCaptureButton = Widgets::Button::make("Add capture", [this]() {
						try {
							this->addCapture();
						}
						RULR_CATCH_ALL_TO_ALERT

					}, ' ');
					addCaptureButton->setHeight(100.0f);
					inspector->add(addCaptureButton);
					inspector->add(Widgets::LiveValue<size_t>::make("Data points", [this]() {
						return this->dataPoints.size();
					}));
					inspector->add(Widgets::Button::make("Clear captures", [this]() {
						this->dataPoints.clear();
					}));

					auto calibrateButton = Widgets::Button::make("Calibrate", [this]() {
						try {
							this->calibrate();
						}
						RULR_CATCH_ALL_TO_ALERT

					}, OF_KEY_RETURN);
					calibrateButton->setHeight(100.0f);
					inspector->add(calibrateButton);

					inspector->add(Widgets::LiveValue<float>::make("Residual", [this]() {
						return this->residual;
					}));
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
				ofxCvGui::PanelPtr MovingHeadToWorld::getView() {
					return this->view;
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

					this->dataPoints.push_back(dataPoint);
					Utils::playSuccessSound();
				}

				//---------
				class Model : public ofxNonLinearFit::Models::Base<MovingHeadToWorld::DataPoint, Model> {
				public:
					Model() {
						//Fit needs to call this constructor at the start
					}

					Model(const ofVec3f & initialPosition, const ofVec3f & initialRotationEuler) {
						this->initialPosition = initialPosition;
						this->initialRotationEuler = initialRotationEuler;
					}

					typedef MovingHeadToWorld::DataPoint DataPoint;
					unsigned int getParameterCount() const override {
						return 6;
					}

					void resetParameters() override {
						auto parameters = this->getParameters();
						*parameters++ = 0;
						*parameters++ = 0;
						*parameters++ = 0;
						*parameters++ = 0;
						*parameters++ = 0;
						*parameters++ = 0;
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

					void evaluate(DataPoint & point) const {
						auto pointInObjectSpace = point.world * this->transform.getInverse();
						point.panTilt = DMX::MovingHead::getPanTiltForTargetInObjectSpace(pointInObjectSpace);
					}

					void cacheModel() override {
						if (this->isReady()) {
							auto parameters = this->getParameters();
							
							auto position = ofVec3f(*parameters++, *parameters++, *parameters++);
							auto rotationEuler = ofVec3f(*parameters++, *parameters++, *parameters++);
							auto rotationQuat = toOf(glm::quat(toGLM(rotationEuler)));
							this->transform = ofMatrix4x4::newRotationMatrix(rotationQuat) * ofMatrix4x4::newTranslationMatrix(position);
						}
					}

					const ofMatrix4x4 & getTransform() {
						return this->transform;
					}
				protected:
					ofVec3f initialPosition;
					ofVec3f initialRotationEuler;
					ofMatrix4x4 transform;
				};

				//---------
				void MovingHeadToWorld::calibrate() {
					this->throwIfMissingAConnection<DMX::MovingHead>();
					auto movingHead = this->getInput<DMX::MovingHead>();

					auto fit = ofxNonLinearFit::Fit<Model>();
					auto model = Model(movingHead->getPosition(), movingHead->getRotationEuler());

					double residual;
					fit.optimise(model, &this->dataPoints, &residual);

					movingHead->setTransform(model.getTransform());
					this->residual = residual;

					Utils::playSuccessSound();
				}
			}
		}
	}
}